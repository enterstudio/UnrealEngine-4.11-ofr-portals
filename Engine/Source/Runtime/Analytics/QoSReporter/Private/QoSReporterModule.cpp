// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "QoSReporterPrivatePCH.h"

#include "Core.h"
#include "Json.h"
#include "Analytics.h"
#include "Http.h"
#include "EngineVersion.h"
#include "QoSReporter.h"

DEFINE_LOG_CATEGORY(LogQoSReporter);

IMPLEMENT_MODULE(FQoSReporterModule, QoSReporter);

FString FQoSReporterModule::Config::GetDefaultAppVersion()
{ 
	return FString::Printf(TEXT("UE4-CL-%d"), FEngineVersion::Current().GetChangelist());
}

FString FQoSReporterModule::Config::GetDefaultAppEnvironment() 
{
	return FAnalytics::ToString(FAnalytics::Get().GetBuildType()); 
}

class FAnalyticsProviderQoSReporter : public IAnalyticsProvider
{
public:
	FAnalyticsProviderQoSReporter(const FQoSReporterModule::Config& ConfigValues);

	/** This provider does not have a concept of sessions */
	virtual bool StartSession(const TArray<FAnalyticsEventAttribute>& Attributes) override { return true; };
	/** This provider does not have a concept of sessions */
	virtual void EndSession() override {};
	/** This provider is not supposed to send many events, and due to nature of QoS we don't want to cache them */
	virtual void FlushEvents() override {};

	/** This provider is not using user IDs */
	virtual void SetUserID(const FString& InUserID) override {};
	/** This provider is not using user IDs */
	virtual FString GetUserID() const override { checkf(false, TEXT("FAnalyticsProviderQoSReporter does not use user ids"));  return TEXT("UnknownUserId"); };

	/** This provider does not have a concept of sessions */
	virtual FString GetSessionID() const override { checkf(false, TEXT("FAnalyticsProviderQoSReporter is not session based"));  return TEXT("UnknownSessionId"); };
	/** This provider does not have a concept of sessions */
	virtual bool SetSessionID(const FString& InSessionID) override { return false; };

	virtual void RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes) override;
	virtual ~FAnalyticsProviderQoSReporter();

	FString GetAPIKey() const { return APIKey; }

private:

	/** API key (also known as "upload type" on data router) */
	FString APIKey;
	/** API Server to use (also known as "endpoint"). */
	FString APIServer;
	/** The AppVersion to use. */
	FString AppVersion;
	/** The AppEnvironment to use. */
	FString AppEnvironment;
	/** The upload type to use. */
	FString UploadType;

	/**
	 * Delegate called when an event Http request completes
	 */
	void EventRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
};

void FQoSReporterModule::StartupModule()
{
	// FQoSReporter::Initialize() is expected to be called by game code with proper config
}

void FQoSReporterModule::ShutdownModule()
{
	FQoSReporter::Shutdown();
}

TSharedPtr<IAnalyticsProvider> FQoSReporterModule::CreateAnalyticsProvider(const FAnalytics::FProviderConfigurationDelegate& GetConfigValue) const
{
	if (GetConfigValue.IsBound())
	{
		Config ConfigValues;
		ConfigValues.APIServer = GetConfigValue.Execute(Config::GetKeyNameForAPIServer(), true);
		ConfigValues.APIKey = GetConfigValue.Execute(Config::GetKeyNameForAPIKey(), false);
		ConfigValues.AppVersion = GetConfigValue.Execute(Config::GetKeyNameForAppVersion(), false);
		ConfigValues.AppEnvironment = GetConfigValue.Execute(Config::GetKeyNameForAppEnvironment(), false);
		ConfigValues.UploadType = GetConfigValue.Execute(Config::GetKeyNameForUploadType(), false);
		return CreateAnalyticsProvider(ConfigValues);
	}
	else
	{
		UE_LOG(LogQoSReporter, Warning, TEXT("CreateAnalyticsProvider called with an unbound delegate"));
	}
	return nullptr;
}

TSharedPtr<IAnalyticsProvider> FQoSReporterModule::CreateAnalyticsProvider(const Config& ConfigValues) const
{
	return TSharedPtr<IAnalyticsProvider>(new FAnalyticsProviderQoSReporter(ConfigValues));
}

/**
 * Perform any initialization.
 */
FAnalyticsProviderQoSReporter::FAnalyticsProviderQoSReporter(const FQoSReporterModule::Config& ConfigValues)
{
	UE_LOG(LogQoSReporter, Verbose, TEXT("Initializing QoS Reporter"));

	APIKey = ConfigValues.APIKey;
	if (APIKey.IsEmpty())
	{
		UE_LOG(LogQoSReporter, Error, TEXT("QoS API key is not configured, no QoS metrics will be reported."));
	}

	APIServer = ConfigValues.APIServer;
	if (APIServer.IsEmpty())
	{
		UE_LOG(LogQoSReporter, Error, TEXT("QoS API server is not configured, no QoS metrics will be reported."));
	}

	AppVersion = ConfigValues.AppVersion;
	if (AppVersion.IsEmpty())
	{
		AppVersion = ConfigValues.GetDefaultAppVersion();
	}

	AppEnvironment = ConfigValues.AppEnvironment;
	if (AppEnvironment.IsEmpty())
	{
		AppEnvironment = ConfigValues.GetDefaultAppEnvironment();
	}

	UploadType = ConfigValues.UploadType;
	if (UploadType.IsEmpty())
	{
		UploadType = ConfigValues.GetDefaultUploadType();
	}

	UE_LOG(LogQoSReporter, Log, TEXT("APIKey = '%s'. APIServer = '%s'. AppVersion = '%s'. AppEnvironment = '%s'"), *APIKey, *APIServer, *AppVersion, *AppEnvironment);
}

FAnalyticsProviderQoSReporter::~FAnalyticsProviderQoSReporter()
{
	UE_LOG(LogQoSReporter, Verbose, TEXT("Destroying QoS Reporter"));
	EndSession();
}

void FAnalyticsProviderQoSReporter::RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (APIKey.IsEmpty() || APIServer.IsEmpty())
	{
		return;
	}

	// encode params as Json
	if (ensure(FModuleManager::Get().IsModuleLoaded("HTTP")))
	{
		FString Payload;

		FDateTime CurrentTime = FDateTime::UtcNow();

		TSharedRef< TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR> >::Create(&Payload);
		JsonWriter->WriteObjectStart();
		JsonWriter->WriteArrayStart(TEXT("Events"));

		// write just a single event
		JsonWriter->WriteObjectStart();
		JsonWriter->WriteValue(TEXT("EventName"), EventName);

		if (Attributes.Num() > 0)
		{
			// optional attributes for this event
			for (int32 AttrIdx = 0; AttrIdx < Attributes.Num(); AttrIdx++)
			{
				const FAnalyticsEventAttribute& Attr = Attributes[AttrIdx];
				JsonWriter->WriteValue(Attr.AttrName, Attr.AttrValue);
			}
		}
		JsonWriter->WriteObjectEnd();

		JsonWriter->WriteArrayEnd();
		JsonWriter->WriteObjectEnd();
		JsonWriter->Close();

		FString URLPath = FString::Printf(TEXT("?AppID=%s&AppVersion=%s&AppEnvironment=%s&UploadType=%s"),
			*FGenericPlatformHttp::UrlEncode(APIKey),
			*FGenericPlatformHttp::UrlEncode(AppVersion),
			*FGenericPlatformHttp::UrlEncode(AppEnvironment),
			*FGenericPlatformHttp::UrlEncode(UploadType)
			);

		// Recreate the URLPath for logging because we do not want to escape the parameters when logging.
		// We cannot simply UrlEncode the entire Path after logging it because UrlEncode(Params) != UrlEncode(Param1) & UrlEncode(Param2) ...
		UE_LOG(LogQoSReporter, VeryVerbose, TEXT("[%s] QoS URL:%s?AppID=%s&AppVersion=%s&AppEnvironment=%s&UploadType=%s. Payload:%s"),
			*APIKey,
			*APIServer,
			*APIKey,
			*AppVersion,
			*AppEnvironment,
			*UploadType,
			*Payload);

		// Create/send Http request for an event
		TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));

		HttpRequest->SetURL(APIServer + URLPath);
		HttpRequest->SetVerb(TEXT("POST"));
		HttpRequest->SetContentAsString(Payload);
		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FAnalyticsProviderQoSReporter::EventRequestComplete);
		HttpRequest->ProcessRequest();

	}
}

void FAnalyticsProviderQoSReporter::EventRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (bSucceeded && HttpResponse.IsValid())
	{
		// normal operation is silent, but any problems are reported as warnings
		if (EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
		{
			UE_LOG(LogQoSReporter, VeryVerbose, TEXT("QoS response for [%s]. Code: %d. Payload: %s"),
				*HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString());
		}
		else
		{
			UE_LOG(LogQoSReporter, Warning, TEXT("Bad QoS response for [%s] - code: %d. Payload: %s"),
				*HttpRequest->GetURL(), HttpResponse->GetResponseCode(), *HttpResponse->GetContentAsString());
		}
	}
	else
	{
		// if we cannot report QoS metrics this is pretty bad; report at least a warning
		UE_LOG(LogQoSReporter, Warning, TEXT("QoS response for [%s]. No response"), *HttpRequest->GetURL());
	}
}

