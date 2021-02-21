#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void merge_flash(char *binfile,char *flashfile,int flash_pos)
{
    FILE *fbin;
    FILE *fflash;
    unsigned char *tmp_data;
    int j=0;

    int file_size=0;
    int flash_size=0;

    fbin = fopen(binfile, "rb");
    if (fbin == NULL) {
        printf("   Can't open '%s' for reading.\n", binfile);
		return;
	}

    if (fseek(fbin, 0 , SEEK_END) != 0) {
        printf("   Can't seek end of '%s'.\n", binfile);
       /* Handle Error */
    }
    file_size = ftell(fbin);

    if (fseek(fbin, 0 , SEEK_SET) != 0) {
      /* Handle Error */
    }

    fflash  = fopen(flashfile, "rb+");
    if (fflash == NULL) {
        printf("   Can't open '%s' for writing.\n", flashfile);
        return;
    }
    if (fseek(fflash, 0 , SEEK_END) != 0) {
          printf("   Can't seek end of '%s'.\n", flashfile);
       /* Handle Error */
    }
    flash_size = ftell(fflash);
    rewind(fflash);
    fseek(fflash,flash_pos,SEEK_SET);


    tmp_data=malloc((1+file_size)*sizeof(char));

    if (file_size<=0) {
      printf("Not able to get file size %s",binfile);
    }

    int len_read=fread(tmp_data,sizeof(char),file_size,fbin);
    int len_write=fwrite(tmp_data,sizeof(char),file_size,fflash);

    if (len_read!=len_write) {
      printf("Not able to merge %s, %d bytes read,%d to write,%d file_size\n",binfile,len_read,len_write,file_size);
    }

    fclose(fbin);

    fclose(fflash);

    free(tmp_data);
}


int main(int argc,char *argv[])
{
    // Flash size is 4MB but we need only 2MB for S and NS FW
    system("dd if=/dev/zero bs=2M count=1 > m33_flash.bin");
    // Add Secure image primary slot
    merge_flash("./cmake_build/install/outputs/MPS2/AN521/tfm_s_signed.bin","m33_flash.bin",0x0);
    // Add Non-secure image primary slot
    merge_flash("./RTOSDemo-signed.bin","m33_flash.bin",0x80000);
    // the rest is empty
}

