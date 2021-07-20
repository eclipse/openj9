/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/
#if defined(LINUX)
#include <criu/criu.h>
#include <fcntl.h>
#include <errno.h>
#endif /* defined(LINUX) */

#include "jni.h"
#include "j9.h"
#include "jclprots.h"
#include "j9jclnls.h"
#include "ut_j9jcl.h"

#define STRING_BUFFER_SIZE 256

static void
setupJNIFieldIDs(JNIEnv *env)
{
	J9VMThread *currentThread = (J9VMThread*)env;
	J9JavaVM *vm = currentThread->javaVM;
	J9InternalVMFunctions *vmFuncs = vm->internalVMFunctions;
	J9CRIUGlobals *criuGlobals = NULL;
	jclass criuResultTypeClass = NULL;
	jclass criuResultClass = NULL;
	jclass criuSupportClass = NULL;
	PORT_ACCESS_FROM_VMC(currentThread);

	criuGlobals = (J9CRIUGlobals *)j9mem_allocate_memory(sizeof(J9CRIUGlobals), OMRMEM_CATEGORY_VM);
	if (NULL == criuGlobals) {
		vmFuncs->setNativeOutOfMemoryError(currentThread, 0, 0);
		goto done;
	}
	memset(criuGlobals, 0, sizeof(*criuGlobals));

	criuResultTypeClass = (*env)->FindClass(env, "org/eclipse/openj9/criu/CRIUSupport$CRIUResultType");
	Assert_JCL_notNull(criuResultTypeClass);
	criuGlobals->criuResultTypeClass = (*env)->NewGlobalRef(env, criuResultTypeClass);

	criuResultClass = (*env)->FindClass(env, "org/eclipse/openj9/criu/CRIUSupport$CRIUResult");
	Assert_JCL_notNull(criuResultClass);
	criuGlobals->criuResultClass = (*env)->NewGlobalRef(env, criuResultClass);

	criuSupportClass = (*env)->FindClass(env, "org/eclipse/openj9/criu/CRIUSupport");
	Assert_JCL_notNull(criuSupportClass);
	criuGlobals->criuSupportClass = (*env)->NewGlobalRef(env, criuSupportClass);

	if (NULL != criuGlobals->criuResultTypeClass
		&& NULL != criuGlobals->criuResultClass
		&& NULL != criuGlobals->criuSupportClass
	) {
		criuGlobals->criuSupportSuccess = (*env)->GetStaticFieldID(env, criuGlobals->criuResultTypeClass, "SUCCESS", "Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;");
		Assert_JCL_notNull(criuGlobals->criuSupportSuccess);
		criuGlobals->criuSupportUnsupportedOperation  = (*env)->GetStaticFieldID(env, criuGlobals->criuResultTypeClass, "UNSUPPORTED_OPERATION", "Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;");
		Assert_JCL_notNull(criuGlobals->criuSupportUnsupportedOperation);
		criuGlobals->criuSupportInvalidArguments = (*env)->GetStaticFieldID(env, criuGlobals->criuResultTypeClass, "INVALID_ARGUMENTS", "Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;");
		Assert_JCL_notNull(criuGlobals->criuSupportInvalidArguments);
		criuGlobals->criuSupportSystemCheckpointFailure = (*env)->GetStaticFieldID(env, criuGlobals->criuResultTypeClass, "SYSTEM_CHECKPOINT_FAILURE", "Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;");
		Assert_JCL_notNull(criuGlobals->criuSupportSystemCheckpointFailure);
		criuGlobals->criuSupportJVMCheckpointFailure = (*env)->GetStaticFieldID(env, criuGlobals->criuResultTypeClass, "JVM_CHECKPOINT_FAILURE", "Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;");
		Assert_JCL_notNull(criuGlobals->criuSupportJVMCheckpointFailure);
		criuGlobals->criuSupportJVMRestoreFailure = (*env)->GetStaticFieldID(env, criuGlobals->criuResultTypeClass, "JVM_RESTORE_FAILURE", "Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;");
		Assert_JCL_notNull(criuGlobals->criuSupportJVMRestoreFailure);

		criuGlobals->criuResultInit = (*env)->GetMethodID(env, criuGlobals->criuResultClass, "<init>", "(Lorg/eclipse/openj9/criu/CRIUSupport$CRIUResultType;Ljava/lang/Throwable;)V");
		Assert_JCL_notNull(criuGlobals->criuResultInit);

		criuGlobals->criuSupportImagesDir = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "imagesDir", "Ljava/lang/String;");
		Assert_JCL_notNull(criuGlobals->criuSupportImagesDir);
		criuGlobals->criuSupportLeaveRunning  = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "leaveRunning", "Z");
		Assert_JCL_notNull(criuGlobals->criuSupportLeaveRunning);
		criuGlobals->criuSupportShellJob = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "shellJob", "Z");
		Assert_JCL_notNull(criuGlobals->criuSupportShellJob);
		criuGlobals->criuSupportExtUnixSupport = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "extUnixSupport", "Z");
		Assert_JCL_notNull(criuGlobals->criuSupportExtUnixSupport);
		criuGlobals->criuSupportLogLevel = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "logLevel", "I");
		Assert_JCL_notNull(criuGlobals->criuSupportLogLevel);
		criuGlobals->criuSupportLogFile = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "logFile", "Ljava/lang/String;");
		Assert_JCL_notNull(criuGlobals->criuSupportLogFile);
		criuGlobals->criuSupportFileLocks = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "fileLocks", "Z");
		Assert_JCL_notNull(criuGlobals->criuSupportFileLocks);
		criuGlobals->criuSupportWorkDir = (*env)->GetFieldID(env, criuGlobals->criuSupportClass, "workDir", "Ljava/lang/String;");
		Assert_JCL_notNull(criuGlobals->criuSupportWorkDir);
	} else {
		vmFuncs->internalEnterVMFromJNI(currentThread);
		vmFuncs->setNativeOutOfMemoryError(currentThread, 0, 0);
		vmFuncs->internalExitVMToJNI(currentThread);
	}
done:
	vm->criuGlobals = criuGlobals;
}

static jobject
constructResult(JNIEnv *env, jfieldID resultType, jthrowable throwable)
{
	J9VMThread *currentThread = (J9VMThread*)env;
	J9JavaVM *vm = currentThread->javaVM;

	jobject criuResult = (*env)->NewObject(env,
			vm->criuGlobals->criuResultClass,
			vm->criuGlobals->criuResultInit,
			(*env)->GetStaticObjectField(env, vm->criuGlobals->criuResultTypeClass, resultType),
			throwable);

	return criuResult;
}

jboolean JNICALL
Java_org_eclipse_openj9_criu_CRIUSupport_isCRIUSupportEnabledImpl(JNIEnv *env, jclass unused)
{
	J9VMThread *currentThread = (J9VMThread *) env;
	J9JavaVM *vm = currentThread->javaVM;
	jboolean res = JNI_FALSE;

	if (vm->internalVMFunctions->isCRIUSupportEnabled(currentThread)) {
#if defined(LINUX)
		if (0 == criu_init_opts()) {
			res = JNI_TRUE;
		}
#endif /* defined(LINUX) */
	}
	setupJNIFieldIDs(env);

	return res;
}

jboolean JNICALL
Java_org_eclipse_openj9_criu_CRIUSupport_isCheckpointAllowed(JNIEnv *env, jclass unused)
{
	J9VMThread *currentThread = (J9VMThread *) env;
	jboolean res = JNI_FALSE;

	if (currentThread->javaVM->internalVMFunctions->isCheckpointAllowed(currentThread)) {
		res = JNI_TRUE;
	}

	return res;
}

static BOOLEAN
getNativeString(J9VMThread *currentThread, j9object_t javaString, char **nativeString, jfieldID *resultType)
{
	J9JavaVM *vm = currentThread->javaVM;
	J9InternalVMFunctions *vmFuncs = vm->internalVMFunctions;
	char mutf8StringBuf[STRING_BUFFER_SIZE];
	char *mutf8String = NULL;
	UDATA mutf8StringSize = 0;
	UDATA requiredConvertedStringSize = 0;
	BOOLEAN res = TRUE;
	PORT_ACCESS_FROM_VMC(currentThread);
	OMRPORT_ACCESS_FROM_J9PORT(PORTLIB);

	mutf8String = vmFuncs->copyStringToUTF8WithMemAlloc(currentThread, javaString, J9_STR_NULL_TERMINATE_RESULT, "", 0, mutf8StringBuf, STRING_BUFFER_SIZE, &mutf8StringSize);
	if (NULL == mutf8String) {
		vmFuncs->setNativeOutOfMemoryError(currentThread, 0, 0);
		*resultType = vm->criuGlobals->criuSupportJVMCheckpointFailure;
		res = FALSE;
		goto free;
	}

	/* get the required size */
	requiredConvertedStringSize = omrstr_convert(J9STR_CODE_MUTF8, J9STR_CODE_PLATFORM_RAW,
					mutf8String,
					mutf8StringSize,
					*nativeString,
					0);

	if (requiredConvertedStringSize < 0) {
		vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_FAILED_TO_CONVERT_JAVA_STRING, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, requiredConvertedStringSize);
		*resultType = vm->criuGlobals->criuSupportInvalidArguments;
		res = FALSE;
		goto free;
	}

	/* Add 1 for NUL terminator */
	requiredConvertedStringSize += 1;

	if (requiredConvertedStringSize > STRING_BUFFER_SIZE) {
		*nativeString = (char*)j9mem_allocate_memory(requiredConvertedStringSize, OMRMEM_CATEGORY_VM);
		if (NULL == *nativeString) {
			vmFuncs->setNativeOutOfMemoryError(currentThread, 0, 0);
			*resultType = vm->criuGlobals->criuSupportJVMCheckpointFailure;
			res = FALSE;
			goto free;
		}
	}

	(*nativeString)[requiredConvertedStringSize - 1] = '\0';

	/* convert the string */
	requiredConvertedStringSize = omrstr_convert(J9STR_CODE_MUTF8, J9STR_CODE_PLATFORM_RAW,
					mutf8String,
					mutf8StringSize,
					*nativeString,
					requiredConvertedStringSize);

	if (requiredConvertedStringSize < 0) {
		vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_FAILED_TO_CONVERT_JAVA_STRING, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, requiredConvertedStringSize);
		*resultType = vm->criuGlobals->criuSupportInvalidArguments;
		res = FALSE;
		goto free;
	}

free:
	if (mutf8String != mutf8StringBuf) {
		j9mem_free_memory(mutf8String);
	}

	return res;
}


jobject JNICALL
Java_org_eclipse_openj9_criu_CRIUSupport_checkpointJVMImpl(JNIEnv *env, jobject criuSupport)
{
	J9VMThread *currentThread = (J9VMThread*)env;
	J9JavaVM *vm = currentThread->javaVM;
	J9InternalVMFunctions *vmFuncs = vm->internalVMFunctions;

	jthrowable currentExceptionLocalRef = NULL;
	jfieldID resultType = vm->criuGlobals->criuSupportUnsupportedOperation;

	if (vmFuncs->isCheckpointAllowed(currentThread)) {
#if defined(LINUX)
		j9object_t cpDir = NULL;
		j9object_t log = NULL;
		j9object_t wrkDir = NULL;
		IDATA dirFD = 0;
		IDATA workDirFD = 0;
		char directoryBuf[STRING_BUFFER_SIZE];
		char *directoryChars = directoryBuf;
		char logFileBuf[STRING_BUFFER_SIZE];
		char *logFileChars = logFileBuf;
		char workDirBuf[STRING_BUFFER_SIZE];
		char *workDirChars = workDirBuf;
		IDATA systemReturnCode = 0;
		jstring imagesDir = (*env)->GetObjectField(env, criuSupport, vm->criuGlobals->criuSupportImagesDir);
		jboolean leaveRunning = (*env)->GetBooleanField(env, criuSupport, vm->criuGlobals->criuSupportLeaveRunning);
		jboolean shellJob = (*env)->GetBooleanField(env, criuSupport, vm->criuGlobals->criuSupportShellJob);
		jboolean extUnixSupport = (*env)->GetBooleanField(env, criuSupport, vm->criuGlobals->criuSupportExtUnixSupport);
		jint logLevel = (*env)->GetIntField(env, criuSupport, vm->criuGlobals->criuSupportLogLevel);
		jstring logFile = (*env)->GetObjectField(env, criuSupport, vm->criuGlobals->criuSupportLogFile);
		jboolean fileLocks = (*env)->GetBooleanField(env, criuSupport, vm->criuGlobals->criuSupportFileLocks);
		jstring workDir = (*env)->GetObjectField(env, criuSupport, vm->criuGlobals->criuSupportWorkDir);
		PORT_ACCESS_FROM_VMC(currentThread);

		vmFuncs->internalEnterVMFromJNI(currentThread);

		Assert_JCL_notNull(imagesDir);
		cpDir = J9_JNI_UNWRAP_REFERENCE(imagesDir);
		if (NULL != logFile) {
			log = J9_JNI_UNWRAP_REFERENCE(logFile);
		}

		if (FALSE == getNativeString(currentThread, cpDir, &directoryChars, &resultType)) {
			/* errors and return codes are set in the helper */
			goto freeDir;
		}

		if (NULL != logFile) {
			if (FALSE == getNativeString(currentThread, log, &logFileChars, &resultType)) {
				/* errors and return codes are set in the helper */
				goto freeLog;
			}
		}

		dirFD = open(directoryChars, O_DIRECTORY);
		if (dirFD < 0) {
			systemReturnCode = errno;
			vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_FAILED_TO_OPEN_DIR, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, systemReturnCode);
			resultType = vm->criuGlobals->criuSupportInvalidArguments;
			goto freeLog;
		}

		if (NULL != workDir) {
			wrkDir = J9_JNI_UNWRAP_REFERENCE(workDir);
			if (FALSE == getNativeString(currentThread, wrkDir, &workDirChars, &resultType)) {
				/* errors and return codes are set in the helper */
				goto freeWorkDir;
			}
			
			workDirFD = open(workDirChars, O_DIRECTORY);
			if (workDirFD < 0) {
				systemReturnCode = errno;
				vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_FAILED_TO_OPEN_DIR, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, systemReturnCode);
				resultType = vm->criuGlobals->criuSupportInvalidArguments;
				goto freeWorkDir;
			}
		}

		systemReturnCode = criu_init_opts();
		if (0 != systemReturnCode) {
			vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_INIT_FAILED, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, systemReturnCode);
			resultType = vm->criuGlobals->criuSupportSystemCheckpointFailure;
			goto closeWorkDirFD;
		}

		criu_set_images_dir_fd(dirFD);
		criu_set_shell_job(FALSE != shellJob);
		if (logLevel > 0) {
			criu_set_log_level((int)logLevel);
		}
		if (NULL != logFile) {
			criu_set_log_file(logFileChars);
		}
		criu_set_leave_running(FALSE != leaveRunning);
		criu_set_ext_unix_sk(FALSE != extUnixSupport);
		criu_set_file_locks(FALSE != fileLocks);
		if (NULL != workDir) {
			criu_set_work_dir_fd(workDirFD);
		}

		vmFuncs->acquireExclusiveVMAccess(currentThread);

		if (FALSE == vmFuncs->jvmCheckpointHooks(currentThread)) {
			resultType = vm->criuGlobals->criuSupportJVMCheckpointFailure;
			goto releaseExclusive;
		}

		systemReturnCode = criu_dump();
		if (systemReturnCode < 0) {
			vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_DUMP_FAILED, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, systemReturnCode);
			resultType = vm->criuGlobals->criuSupportSystemCheckpointFailure;
			goto releaseExclusive;
		}

		/* We can only end up here if the CRIU restore was successful */

		if (FALSE == vmFuncs->jvmRestoreHooks(currentThread)) {
			resultType = vm->criuGlobals->criuSupportJVMRestoreFailure;
			goto releaseExclusive;
		}

		resultType = vm->criuGlobals->criuSupportSuccess;

releaseExclusive:
		vmFuncs->releaseExclusiveVMAccess(currentThread);
closeWorkDirFD:
		if (0 != close(workDirFD)) {
			systemReturnCode = errno;
			resultType = vm->criuGlobals->criuSupportJVMRestoreFailure;
			vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_FAILED_TO_CLOSE_DIR, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, systemReturnCode);
		}
freeWorkDir:
		if (0 != close(dirFD)) {
			systemReturnCode = errno;
			resultType = vm->criuGlobals->criuSupportJVMRestoreFailure;
			vmFuncs->setCurrentExceptionNLSWithArgs(currentThread, J9NLS_JCL_CRIU_FAILED_TO_CLOSE_DIR, J9VMCONSTANTPOOL_JAVALANGINTERNALERROR, systemReturnCode);
		}
		if (workDirChars != workDirBuf) {
			j9mem_free_memory(workDirChars);
		}
freeLog:
		if (logFileChars != logFileBuf) {
			j9mem_free_memory(logFileChars);
		}
freeDir:
		if (directoryChars != directoryBuf) {
			j9mem_free_memory(directoryChars);
		}
		if (NULL != currentThread->currentException) {
			currentExceptionLocalRef = (jthrowable) vmFuncs->j9jni_createLocalRef(env, currentThread->currentException);
			currentThread->currentException = NULL;

			if (NULL == currentExceptionLocalRef) {
				/* nothing else to do */
				vmFuncs->setNativeOutOfMemoryError(currentThread, 0, 0);
			}
		}

		vmFuncs->internalExitVMToJNI(currentThread);
#endif /* defined(LINUX) */
	}

	return constructResult(env, resultType, currentExceptionLocalRef);
}
