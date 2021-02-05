#include <kernel_ex.h>
#include <message_dialog.h>
#include <system_service_ex.h>

#define BUF_SIZE (1024 * 1024)
#define SOURCE_CORES_PATH "/app0/cores"
#define TARGET_CORES_PATH "/data/self/retroarch/cores"

char sandbox_lib_path[32];

struct Module {
  char *path;
  char *name;
  SceKernelModule handle;
};

struct Module modules[] = {
  { .path = sandbox_lib_path, .name = "libSceSysCore.sprx" , .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceMbus.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceIpmi.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceSystemService.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceUserService.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceNetCtl.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceNet.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceAudioOut.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libScePad.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceMsgDialog.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceRegMgr.sprx", .handle = -1 },
  { .path = sandbox_lib_path, .name = "libSceCommonDialog.sprx", .handle = -1 },
};

int load_modules(void) {
  char file_path[SCE_KERNEL_MAX_NAME_LENGTH];
  int i, res;
  int len = sizeof(modules) / sizeof(modules[0]);

  for (i = 0; i < len; i++) {
	  snprintf(file_path, sizeof(file_path), "%s/%s", modules[i].path, modules[i].name);
	  modules[i].handle = sceKernelLoadStartModule(file_path, 0, NULL, 0, NULL, &res);
    if(res < 0) {
      return(res);
    }
  }

  return(0);
}

int unload_modules(void) {
  int i, res;
  int len = sizeof(modules) / sizeof(modules[0]);

  for (i = len - 1; i > 3; i--) {
    if (modules[i].handle >= 0) {
      sceKernelStopUnloadModule(modules[i].handle, 0, NULL, 0, NULL, &res);
      if (res < 0) {
        return(res);
      }
    }
    modules[i].handle = -1;
  }

  return(0);
}

int main(int argc, const char* const argv[]) {
  size_t bytes;
  char buffer[BUF_SIZE], msg_prog[128], source_file[SCE_KERNEL_MAX_NAME_LENGTH], target_file[SCE_KERNEL_MAX_NAME_LENGTH];
  static const char msg_install[] = "Press OK to install the core files.";
  static const char msg_done[] = "Done installing cores. Press OK to close.";
  const char *sandbox_word = NULL;
	int ret, file_count, copied_count;
  DIR * dirp;
  struct dirent *entry;
  FILE *source, *target;
  SceMsgDialogParam param;
  SceMsgDialogUserMessageParam userMsgParam;
  SceMsgDialogProgressBarParam  progBarParam;
  SceCommonDialogStatus stat;
  SceMsgDialogResult result;

  sandbox_word = sceKernelGetFsSandboxRandomWord();
  snprintf(sandbox_lib_path, sizeof(sandbox_lib_path), "/%s/common/lib", sandbox_word);
  load_modules();
  sceSystemServiceHideSplashScreen();
  ret = sceCommonDialogInitialize();
	ret = sceMsgDialogInitialize();

  // prompt ok message dialog
  sceMsgDialogParamInitialize(&param);
  param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
  sceUserServiceGetInitialUser(&param.userId);
  memset(&userMsgParam, 0, sizeof(userMsgParam));
  userMsgParam.msg = msg_install;
  userMsgParam.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL;
  param.userMsgParam = &userMsgParam;
  ret = sceMsgDialogOpen(&param);

  while(1) {
    sceKernelUsleep(1e3 * 200);
    stat = sceMsgDialogUpdateStatus();
    if (stat == SCE_COMMON_DIALOG_STATUS_RUNNING) {
      continue;
    } else if(stat == SCE_COMMON_DIALOG_STATUS_FINISHED) {
      memset( &result, 0, sizeof(result) );
      sceMsgDialogGetResult( &result );
      break;
    }
  }

  if (result.result == SCE_COMMON_DIALOG_RESULT_OK && result.buttonId == SCE_MSG_DIALOG_BUTTON_ID_OK) {
    (void)result;
  } else if (result.result == SCE_COMMON_DIALOG_RESULT_USER_CANCELED && result.buttonId == SCE_MSG_DIALOG_BUTTON_ID_INVALID) {

    // cancel button pressed, unload modules and exit
    unload_modules();
    sceSystemServiceLoadExec("", NULL);
  }

  // get file count
  file_count = 0;
  dirp = opendir(SOURCE_CORES_PATH);
  while((entry = readdir(dirp)) != NULL) {
    if (entry->d_type == DT_REG) {
      file_count++;
    }
  }

  if (dirp)
    closedir(dirp);

  // prompt install files dialog
  sceMsgDialogParamInitialize( &param );
  param.mode = SCE_MSG_DIALOG_MODE_PROGRESS_BAR;
  sceUserServiceGetInitialUser(&param.userId);
  memset( &progBarParam, 0, sizeof(progBarParam) );
  progBarParam.barType = SCE_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;
  progBarParam.msg = "Installing cores...";
  param.progBarParam = &progBarParam;
  ret = sceMsgDialogOpen(&param) ;

  // make sure the directory exists
  //sceKernelMkdir(TARGET_CORES_PATH, 0777);
  mkdir("/data", 0777);
  mkdir("/data/self", 0777);
  mkdir("/data/self/retroarch", 0777);
  mkdir("/data/self/retroarch/cores", 0777);

  // copy all the files
  copied_count = 0;
  dirp = opendir(SOURCE_CORES_PATH);
  while (1) {
    stat = sceMsgDialogUpdateStatus();
    if( stat == SCE_COMMON_DIALOG_STATUS_RUNNING ) {
      entry = readdir(dirp);
      if (entry != NULL) {
        if (entry->d_type == DT_REG) {
          snprintf(source_file, sizeof(source_file), SOURCE_CORES_PATH "/%s", entry->d_name);
          source = fopen(source_file, "rb");
          if (source) {
            snprintf(target_file, sizeof(target_file), TARGET_CORES_PATH "/%s", entry->d_name);
            target = fopen(target_file, "wb");
            if (target) {
              while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
                fwrite(buffer, 1, bytes, target);
              }
              fclose(target);
              copied_count++;

              // give file executable permissions
              sceKernelChmod(target_file, SCE_KERNEL_S_IRWXU);
            }
            fclose(source);
          }
        }
      }
      snprintf(msg_prog, sizeof(msg_prog), "Installed %d of %d cores...", copied_count, file_count);
			sceMsgDialogProgressBarSetMsg(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, msg_prog);
			sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (uint32_t)((100*copied_count/file_count)));
      if (copied_count >= file_count) {
        sceKernelUsleep(1e6 * 1);
        sceMsgDialogClose();
        continue;
      }
    } else if(stat == SCE_COMMON_DIALOG_STATUS_FINISHED) {
      memset( &result, 0, sizeof(result) );
      sceMsgDialogGetResult( &result );
      break;
    }
    sceKernelUsleep(1e3*200);
  }

  if (dirp)
    closedir(dirp);

  // prompt done dialog
  sceMsgDialogParamInitialize( &param );
  param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
  sceUserServiceGetInitialUser(&param.userId);
  memset( &userMsgParam, 0, sizeof(userMsgParam) );
  userMsgParam.msg = msg_done;
  userMsgParam.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
  param.userMsgParam = &userMsgParam;
  ret = sceMsgDialogOpen(&param) ;

  while(1) {
    sceKernelUsleep(1e3 * 200);
    stat = sceMsgDialogUpdateStatus();
    if (stat == SCE_COMMON_DIALOG_STATUS_RUNNING)
      continue;
    else if(stat == SCE_COMMON_DIALOG_STATUS_FINISHED) {
      memset( &result, 0, sizeof(result) );
      sceMsgDialogGetResult( &result );
      break;
    }
  }

  if (result.result == SCE_COMMON_DIALOG_RESULT_OK && result.buttonId == SCE_MSG_DIALOG_BUTTON_ID_OK) {
    sceMsgDialogTerminate();
  }

  unload_modules();
  sceSystemServiceLoadExec("", NULL);

	return(0);
}

void catchReturnFromMain(int exit_code) {
	/* dummy */
}

