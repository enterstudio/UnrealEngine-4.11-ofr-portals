// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "AndroidJava.h"
#include "AndroidApplication.h"

FJavaClassObject::FJavaClassObject(FName ClassName, const char* CtorSig, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();

	Class = FAndroidApplication::FindJavaClass(ClassName.GetPlainANSIString());
	check(Class);
	jmethodID Constructor = JEnv->GetMethodID(Class, "<init>", CtorSig);
	check(Constructor);
	va_list Params;
	va_start(Params, CtorSig);
	jobject object = JEnv->NewObjectV(Class, Constructor, Params);
	va_end(Params);
	VerifyException();
	check(object);

	// Promote local references to global
	Object = JEnv->NewGlobalRef(object);
	JEnv->DeleteLocalRef(object);
}

FJavaClassObject::~FJavaClassObject()
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	JEnv->DeleteGlobalRef(Object);
}

FJavaClassMethod FJavaClassObject::GetClassMethod(const char* MethodName, const char* FuncSig)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	FJavaClassMethod Method;
	Method.Method = JEnv->GetMethodID(Class, MethodName, FuncSig);
	Method.Name = MethodName;
	Method.Signature = FuncSig;
	// Is method valid?
	checkf(Method.Method, TEXT("Unable to find Java Method %s with Signature %s"), UTF8_TO_TCHAR(MethodName), UTF8_TO_TCHAR(FuncSig));
	return Method;
}

template<>
void FJavaClassObject::CallMethod<void>(FJavaClassMethod Method, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	va_list Params;
	va_start(Params, Method);
	JEnv->CallVoidMethodV(Object, Method.Method, Params);
	va_end(Params);
	VerifyException();
}

template<>
bool FJavaClassObject::CallMethod<bool>(FJavaClassMethod Method, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	va_list Params;
	va_start(Params, Method);
	bool RetVal = JEnv->CallBooleanMethodV(Object, Method.Method, Params);
	va_end(Params);
	VerifyException();
	return RetVal;
}

template<>
int FJavaClassObject::CallMethod<int>(FJavaClassMethod Method, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	va_list Params;
	va_start(Params, Method);
	int RetVal = JEnv->CallIntMethodV(Object, Method.Method, Params);
	va_end(Params);
	VerifyException();
	return RetVal;
}

template<>
jobject FJavaClassObject::CallMethod<jobject>(FJavaClassMethod Method, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	va_list Params;
	va_start(Params, Method);
	jobject val = JEnv->CallObjectMethodV(Object, Method.Method, Params);
	va_end(Params);
	VerifyException();
	jobject RetVal = JEnv->NewGlobalRef(val);
	JEnv->DeleteLocalRef(val);
	return RetVal;
}

template<>
int64 FJavaClassObject::CallMethod<int64>(FJavaClassMethod Method, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	va_list Params;
	va_start(Params, Method);
	int64 RetVal = JEnv->CallLongMethodV(Object, Method.Method, Params);
	va_end(Params);
	VerifyException();
	return RetVal;
}

template<>
FString FJavaClassObject::CallMethod<FString>(FJavaClassMethod Method, ...)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	va_list Params;
	va_start(Params, Method);
	jstring RetVal = static_cast<jstring>(
		JEnv->CallObjectMethodV(Object, Method.Method, Params));
	va_end(Params);
	VerifyException();
	const char * UTFString = JEnv->GetStringUTFChars(RetVal, nullptr);
	FString Result(UTF8_TO_TCHAR(UTFString));
	JEnv->ReleaseStringUTFChars(RetVal, UTFString);
	return Result;
}

jstring FJavaClassObject::GetJString(const FString& String)
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	auto StringCastObj = StringCast<ANSICHAR>(*String);
	jstring local = JEnv->NewStringUTF(StringCastObj.Get());
	jstring result = (jstring)JEnv->NewGlobalRef(local);
	JEnv->DeleteLocalRef(local);
	return result;
}

void FJavaClassObject::VerifyException()
{
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		verify(false && "Java JNI call failed with an exception.");
	}
}
