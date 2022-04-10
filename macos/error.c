// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "error.h"

#include "main.h"
#include "resources.h"
#include "strutil.h"

#include <Dialogs.h>
#include <TextUtils.h>

void ExitError(ErrorCode errCode)
{
	Str255 message;

	GetIndString(message, rSTRS_Errors, errCode);
	if (message[0] == 0) {
		GetIndString(message, rSTRS_Errors, kErrUnknown);
	}
	ParamText(message, NULL, NULL, NULL);
	Alert(rAlrtError, NULL);
	QuitApp();
}

void ExitErrorOS(ErrorCode errCode, short osErr)
{
	Str255 message;

	GetIndString(message, rSTRS_Errors, errCode);
	if (message[0] == 0) {
		GetIndString(message, rSTRS_Errors, kErrUnknown);
	}
	StrAppendFormat(message, "\p\rError code: %d", osErr);
	ParamText(message, NULL, NULL, NULL);
	Alert(rAlrtError, NULL);
	QuitApp();
}

void ExitMemError(void)
{
	ExitErrorOS(kErrOutOfMemory, MemError());
}

void ExitAssert(const unsigned char *file, int line,
                const unsigned char *message)
{
	Str255 dmessage;

	GetIndString(dmessage, rSTRS_Errors, kErrInternal);
	StrAppendFormat(dmessage, "\p\rError at: %S:%d", file, line);
	if (message != NULL) {
		StrAppendFormat(dmessage, "\p: %S", message);
	}
	ParamText(dmessage, NULL, NULL, NULL);
	Alert(rAlrtError, NULL);
	QuitApp();
}

void ShowError(const struct ErrorParams *p)
{
	Str255 message;

	GetIndString(message, rSTRS_Errors, p->err);
	if (message[0] == 0) {
		GetIndString(message, rSTRS_Errors, kErrUnknown);
	} else if (p->strParam != NULL) {
		StrSubstitute(message, p->strParam);
	}
	if (p->osErr != NULL) {
		StrAppendFormat(message, "\p\rError code: %d", p->osErr);
	}
	ParamText(message, NULL, NULL, NULL);
	Alert(rAlrtError, NULL);
}
