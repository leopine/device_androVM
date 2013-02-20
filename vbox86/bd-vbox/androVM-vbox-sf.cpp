#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <VBox/VBoxGuest.h>
#include <iprt/err.h>
#include <VBox/shflsvc.h>
#include <VBGLR3Internal.h>

int connectSharedFolders(int *clientID)
{
    VBoxGuestHGCMConnectInfo Info;
    memset(&Info, 0, sizeof(Info));
    Info.result = VERR_WRONG_ORDER;
    Info.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    strcpy(Info.Loc.u.host.achName, "VBoxSharedFolders");
    Info.u32ClientID = UINT32_MAX;  /* try make valgrind shut up. */

    int iofile = open("/dev/vboxguest",O_RDWR);
    if (!iofile) {
        fprintf(stderr,"Unable to open /dev/vboxguest errno=%d\n",errno);
        return -1;
    }

    int rc = ioctl(iofile, VBOXGUEST_IOCTL_HGCM_CONNECT, &Info);
    if (rc) {
        fprintf(stderr,"Error in ioctl errno=%d\n", errno);
        return -2;
    }

    rc = Info.result;
    if (RT_FAILURE(rc)) {
        fprintf(stderr,"Connect NOK, rc=%d\n",rc);
	return -1;
    }

    *clientID = Info.u32ClientID;

    return iofile;
}

char *converUTF16toUTF8(unsigned short *s_utf16, int utf16_len)
{
    unsigned short *pwsz = s_utf16;
    int cwc = utf16_len;
    char *s_utf8;
    char *pwch;

    s_utf8 = (char *)malloc(utf16_len*2+1);
    pwch = s_utf8;
    if (!pwch)
        return NULL;

    while (cwc > 0) {
        unsigned short wc = *pwsz++; cwc--;
        if (!wc)
            break;
        else if (wc < 0xd800 || wc > 0xdfff)
        {
            if (wc < 0x80)
            {
                *pwch++ = (char)wc;
            }
            else if (wc < 0x800)
            {
                *pwch++ = 0xc0 | (wc >> 6);
                *pwch++ = 0x80 | (wc & 0x3f);
            }
            else if (wc < 0xfffe)
            {
                *pwch++ = 0xe0 | (wc >> 12);
                *pwch++ = 0x80 | ((wc >> 6) & 0x3f);
                *pwch++ = 0x80 | (wc & 0x3f);
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            if (wc >= 0xdc00)
                return NULL;
            if (cwc <= 0)
                return NULL;
            unsigned short wc2 = *pwsz++; cwc--;
            if (wc2 < 0xdc00 || wc2 > 0xdfff)
                return NULL;
            uint32_t CodePoint = 0x10000
                               + (  ((wc & 0x3ff) << 10)
                                  | (wc2 & 0x3ff));
            *pwch++ = 0xf0 | (CodePoint >> 18);
            *pwch++ = 0x80 | ((CodePoint >> 12) & 0x3f);
            *pwch++ = 0x80 | ((CodePoint >>  6) & 0x3f);
            *pwch++ = 0x80 | (CodePoint & 0x3f);
        }
    }

    /* done */
    *pwch = '\0';
    return s_utf8;
}

void listSharedFolders(int fd_ioctl, int clientID, char *mount_root)
{
    int rc;
    VBoxSFQueryMappings Msg;

    Msg.callInfo.result = VERR_WRONG_ORDER;
    Msg.callInfo.u32ClientID = clientID;
    Msg.callInfo.u32Function = SHFL_FN_QUERY_MAPPINGS;
    Msg.callInfo.cParms = 3;

    /* Set the mapping flags. */
    uint32_t u32Flags = 0; /** @todo SHFL_MF_UTF8 is not implemented yet. */
    u32Flags |= SHFL_MF_AUTOMOUNT;
    //u32Flags |= SHFL_MF_UTF8; // Not supported :-(
    VbglHGCMParmUInt32Set(&Msg.flags, u32Flags);

    /*
     * Prepare and get the actual mappings from the host service.
     */
    rc = VINF_SUCCESS;
    uint32_t cMappings = 8; /* Should be a good default value. */
    uint32_t cbSize = cMappings * sizeof(VBGLR3SHAREDFOLDERMAPPING);
    //VBGLR3SHAREDFOLDERMAPPING *ppaMappingsTemp = (PVBGLR3SHAREDFOLDERMAPPING)RTMemAllocZ(cbSize);
    VBGLR3SHAREDFOLDERMAPPING *ppaMappingsTemp = (PVBGLR3SHAREDFOLDERMAPPING)malloc(cbSize);
    if (ppaMappingsTemp == NULL)
        rc = VERR_NO_MEMORY;

    do
    {
        VbglHGCMParmUInt32Set(&Msg.numberOfMappings, cMappings);
        VbglHGCMParmPtrSet(&Msg.mappings, ppaMappingsTemp, cbSize);

        rc = ioctl(fd_ioctl, VBOXGUEST_IOCTL_HGCM_CALL(sizeof(Msg)), &Msg);
        if (rc) {
            fprintf(stderr,"Error in ioctl errno=%d\n", errno);
            return;
        }
        rc = Msg.callInfo.result;

        if (RT_SUCCESS(rc))
        {
            VbglHGCMParmUInt32Get(&Msg.numberOfMappings, &cMappings);

            /* Do we have more mappings than we have allocated space for? */
            if (rc == VINF_BUFFER_OVERFLOW)
            {
                cbSize = cMappings * sizeof(VBGLR3SHAREDFOLDERMAPPING);
                //void *pvNew = RTMemRealloc(ppaMappingsTemp, cbSize);
                void *pvNew = realloc(ppaMappingsTemp, cbSize);
                //AssertPtrBreakStmt(pvNew, rc = VERR_NO_MEMORY);
                ppaMappingsTemp = (PVBGLR3SHAREDFOLDERMAPPING)pvNew;
            }
        }
        else
            fprintf(stderr,"Mappings list error, rc=%d\n", rc);
    } while (rc == VINF_BUFFER_OVERFLOW);

    if (ppaMappingsTemp) {
        uint32_t s;

        for (s=0;s<cMappings;s++) {
            VBoxSFQueryMapName Msg2;

            Msg2.callInfo.result = VERR_WRONG_ORDER;
            Msg2.callInfo.u32ClientID = clientID;
            Msg2.callInfo.u32Function = SHFL_FN_QUERY_MAP_NAME;
            Msg2.callInfo.cParms = 2;

            uint32_t    cbString = sizeof(SHFLSTRING) + SHFL_MAX_LEN;
            PSHFLSTRING pString = (PSHFLSTRING)malloc(cbString);
            if (pString) {
                memset(pString, 0, cbString);
                ShflStringInitBuffer(pString, SHFL_MAX_LEN);

                VbglHGCMParmUInt32Set(&Msg2.root, ppaMappingsTemp[s].u32Root);
                VbglHGCMParmPtrSet(&Msg2.name, pString, cbString);

                rc = ioctl(fd_ioctl, VBOXGUEST_IOCTL_HGCM_CALL(sizeof(Msg2)), &Msg2);
                if (rc) {
                    fprintf(stderr,"Error in ioctl errno=%d\n", errno);
                    continue;
                }
                rc = Msg.callInfo.result;
                if (RT_SUCCESS(rc)) {
                   char *pszName=converUTF16toUTF8((unsigned short *)&(pString->String.utf8[0]), pString->u16Length);

                   if (pszName) {
                       char *mount_cmd;

                       mount_cmd = (char *)malloc(strlen(pszName)+80);
                       if (mount_cmd) {
                           sprintf(mount_cmd, "%s/%s", mount_root, pszName);
                           mkdir(mount_cmd, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
                           sprintf(mount_cmd, "/system/bin/mount.vboxsf \"%s\" \"%s/%s\"", pszName, mount_root, pszName);
                           printf("Mounting %s\n", pszName);
                           system(mount_cmd);
                           free(mount_cmd);
                       }
                       free(pszName);
                   }
                }
                else 
                    fprintf(stderr,"Get share name %d error, rc=%d\n", s, rc);
            }
        }

        free(ppaMappingsTemp);
    }
}

int main(int argc, char *argv[]) 
{
    int fd_ioctl;
    int clientID;

    if (argc!=2) {
        fprintf(stderr, "Bad command line, Usage: %s <root mount point>\n", argv[0]);
        return -1;
    }

    fd_ioctl = connectSharedFolders(&clientID);
    if (fd_ioctl<0) {
	fprintf(stderr, "Unable to open VirtualBox ioctl\n");
        return -1;
    }

    listSharedFolders(fd_ioctl, clientID, argv[1]);
}
