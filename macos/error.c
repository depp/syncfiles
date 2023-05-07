// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "macos/error.h"

#include "lib/defs.h"
#include "macos/main.h"
#include "macos/pstrbuilder.h"
#include "macos/resources.h"

#include <Dialogs.h>
#include <TextUtils.h>

enum {
	// An error of type ^2 occurred.
	kStrErrorCode = 1,
	// Error at: ^1:^2
	kStrErrorAt,
	// Assertion: ^1
	kStrAssertion,
	// An internal error occurred.
	kStrInternal,
	// An unknown error occurred.
	kStrUnknown,
	// (Base value for error messages.)
	kStrMessageBase
};

static void AppendErrorArray(struct PStrBuilder *str, int sep, int strNum,
                             int paramCount, const unsigned char *const *params)
{
	Str255 message;

	if (str->data[0] > 0 && sep != 0) {
		PStrAppendChar(str, sep);
	}
	GetIndString(message, rSTRS_Errors, strNum);
	if (message[0] == 0) {
		PStrAppendChar(str, '?');
	} else {
		PStrAppendSubstitute(str, message, paramCount, params);
	}
}

static void AppendError0(struct PStrBuilder *str, int sep, int strNum)
{
	AppendErrorArray(str, sep, strNum, 0, NULL);
}

static void AppendError1(struct PStrBuilder *str, int sep, int strNum,
                         const unsigned char *strParam)
{
	AppendErrorArray(str, sep, strNum, 1, &strParam);
}

static void AppendError2(struct PStrBuilder *str, int sep, int strNum,
                         const unsigned char *strParam, int intParam)
{
	unsigned char num[16];
	const unsigned char *params[2];

	params[0] = strParam;
	NumToString(intParam, num);
	params[1] = num;
	AppendErrorArray(str, sep, strNum, 2, params);
}

static void ShowErrorAlert(const unsigned char *message, int id)
{
	ParamText(message, NULL, NULL, NULL);
	Alert(id, NULL);
}

void ExitAssert(const unsigned char *file, int line,
                const unsigned char *assertion)
{
	struct PStrBuilder str;

	PStrInit(&str);
	AppendError0(&str, 0, kStrInternal);
	if (file != NULL) {
		AppendError2(&str, '\r', kStrErrorAt, file, line);
	}
	if (assertion != NULL) {
		AppendError1(&str, '\r', kStrAssertion, assertion);
	}
	ShowErrorAlert(str.data, rAlrtFatal);
	QuitApp();
}

void ShowError(ErrorCode err1, ErrorCode err2, short osErr,
               const unsigned char *strParam)
{
	struct PStrBuilder str;

	PStrInit(&str);
	if (err1 != kErrNone) {
		AppendError1(&str, ' ', kStrMessageBase - 1 + err1, strParam);
	}
	if (err2 != kErrNone) {
		AppendError1(&str, ' ', kStrMessageBase - 1 + err2, strParam);
	}
	if (osErr != 0) {
		AppendError2(&str, ' ', kStrErrorCode, NULL, osErr);
	}
	if (str.data[0] == 0) {
		AppendError0(&str, ' ', kStrUnknown);
	}
	ShowErrorAlert(str.data, rAlrtError);
}

void ShowMemError(void)
{
	ShowError(kErrOutOfMemory, kErrNone, MemError(), NULL);
}
