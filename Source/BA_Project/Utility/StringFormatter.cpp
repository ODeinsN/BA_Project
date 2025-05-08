// Fill out your copyright notice in the Description page of Project Settings.


#include "StringFormatter.h"

#include <regex>
#include <string>

void UStringFormatter::ReduceSpaces(const FString& InputText, FString &ReplacementString) 
{
	const std::string s { TCHAR_TO_UTF8(*InputText) };
	const std::string ns { " " };
	const std::regex p { std::regex { "\\s{2,}", std::regex_constants::icase } };

	std::string resultString { std::regex_replace(s, p, ns) };

	ReplacementString = FString { resultString.c_str() };
}
