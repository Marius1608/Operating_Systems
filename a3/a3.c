#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#define REQ_PIPE "REQ_PIPE_23302"
#define RESP_PIPE "RESP_PIPE_23302"
#define SHM_NAME "/9ZNXBn"  
int size_file_map;

typedef struct __attribute__((packed)) {
    char name[11];
    short int type;
    int offset;
    int size;
} SFile;


int main() {

    int fd_req=-1, fd_resp=-1;
    void *shm_addr = NULL;
    unsigned int shm_size = 0;
   
    if (mkfifo(RESP_PIPE, 0600) == -1) {
        perror("ERROR\ncannot create the response pipe");
        return 1;
    }

   
    fd_req = open(REQ_PIPE, O_RDONLY);
    if (fd_req == -1) {
        perror("ERROR\ncannot open the request pipe");
        return 1;
    }

    
    fd_resp = open(RESP_PIPE, O_WRONLY);
    if (fd_resp == -1) {
        perror("ERROR\ncannot open the response pipe");
        close(fd_req);
        return 1;
    }

    if (write(fd_resp, "START#", strlen("START#")*sizeof(char)) == -1) {
        perror("ERROR\ncannot create the response pipe");
        close(fd_req);
        close(fd_resp);
        return 1;
    }

    
    printf("SUCCESS\n");

    while (1) {

        char name[256],file_name[256],caracter='0';
        int shm_fd =-1,length=0,fd_map=-1;
        char *data=NULL;
        

        while(caracter!='#'){
            read(fd_req,&caracter,sizeof(char));
            name[length++]=caracter;
        }
        name[length]='\0';

        if (length > 0) {
        
            
            if (strcmp(name, "PING#") == 0) {

                int numar = 23302; 
                if (write(fd_resp, "PING#PONG#", strlen("PING#PONG#")*sizeof(char)) == -1) {
                    return 1;
                }
                write(fd_resp, &numar, sizeof(numar));


            } else if (strcmp(name, "CREATE_SHM#") == 0) {

                shm_size = 0;
                read(fd_req, &shm_size, sizeof(unsigned int));
                shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);

                if (shm_fd == -1) {
                    write(fd_resp, "CREATE_SHM#ERROR#", strlen("CREATE_SHM#ERROR#")*sizeof(char));
                    return 1;
                }

                if (ftruncate(shm_fd, shm_size) == -1) {
                    write(fd_resp, "CREATE_SHM#ERROR#", strlen("CREATE_SHM#ERROR#")*sizeof(char));
                    shm_unlink(SHM_NAME);
                    return 1;
                }

                shm_addr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (shm_addr == MAP_FAILED) {
                    write(fd_resp, "CREATE_SHM#ERROR#", strlen("CREATE_SHM#ERROR#")*sizeof(char));
                    shm_unlink(SHM_NAME);
                     return 1;
                }

                write(fd_resp, "CREATE_SHM#SUCCESS#", strlen("CREATE_SHM#SUCCESS#")*sizeof(char));


            } else if (strcmp(name, "WRITE_TO_SHM#") == 0) {

                unsigned int offset = 0, value = 0;
                read(fd_req, &offset, sizeof(unsigned int));
                read(fd_req, &value, sizeof(unsigned int));

                if(offset >= 0 && (offset + sizeof(unsigned int)) < shm_size) {
                    memcpy(shm_addr + offset, &value, sizeof(unsigned int));
                    write(fd_resp, "WRITE_TO_SHM#SUCCESS#", strlen("WRITE_TO_SHM#SUCCESS#")*sizeof(char));
                } else {
                   write(fd_resp, "WRITE_TO_SHM#ERROR#", strlen("WRITE_TO_SHM#ERROR#")*sizeof(char));
                   return 1;
                }


            } else if (strcmp(name, "MAP_FILE#") == 0) {

                char c='0';
                while(c!='#'){
                    read(fd_req,&c,sizeof(char));
                    file_name[size_file_map++]=c;
                }
                file_name[size_file_map-1]='\0';

                
                fd_map = open(file_name, O_RDONLY);
                if (fd_map == -1) {
                    write(fd_resp, "MAP_FILE#ERROR#", strlen("MAP_FILE#ERROR#")*sizeof(char));
                    return 1;
                }

                size_file_map=lseek(fd_map,0,SEEK_END);
                lseek(fd_map, 0, SEEK_SET);

                data = (char*)mmap(NULL, size_file_map, PROT_READ , MAP_SHARED, fd_map, 0);
                if (data == (void *)-1) {
                    write(fd_resp, "MAP_FILE#ERROR#", strlen("MAP_FILE#ERROR#")*sizeof(char));
                    return 1;
                }

                write(fd_resp, "MAP_FILE#SUCCESS#", strlen("MAP_FILE#SUCCESS#")*sizeof(char));
                

            } else if(strcmp(name, "READ_FROM_FILE_OFFSET#") == 0) {

                unsigned int offset = 0, no_of_bytes = 0;
                int error=1;

                fd_map = open(file_name, O_RDONLY);
                data = (char*)mmap(NULL, size_file_map, PROT_READ , MAP_SHARED, fd_map, 0);

                read(fd_req, &offset, sizeof(unsigned int));
                read(fd_req, &no_of_bytes, sizeof(unsigned int));
                
                
                if(no_of_bytes+offset>size_file_map || offset+sizeof(unsigned int)>size_file_map || no_of_bytes>size_file_map) error=-1;
                if(shm_addr==NULL || shm_addr==MAP_FAILED || fd_map==-1 || offset<0) error=-1;
            
                
                if(error==-1){
                  write(fd_resp, "READ_FROM_FILE_OFFSET#ERROR#", strlen("READ_FROM_FILE_OFFSET#ERROR#")*sizeof(char));
                  continue;
                }
                else{
                  memcpy(shm_addr,data+offset,no_of_bytes);
                  write(fd_resp, "READ_FROM_FILE_OFFSET#SUCCESS#", strlen("READ_FROM_FILE_OFFSET#SUCCESS#")*sizeof(char));
                }

            } else if(strcmp(name, "READ_FROM_FILE_SECTION#") == 0){

                unsigned int section_no=0,offset=0,no_of_bytes=0;
                int offset_sect=0,size_sect=0,error=1;
                char magic='0',nr_sections='0';
                short int header_size=0,version=0;

                fd_map = open(file_name, O_RDONLY);
                data = (char*)mmap(NULL, size_file_map, PROT_READ , MAP_SHARED, fd_map, 0);

                read(fd_req, &section_no, sizeof(unsigned int));
                read(fd_req, &offset, sizeof(unsigned int));
                read(fd_req, &no_of_bytes, sizeof(unsigned int));
               
                memcpy(&magic, data, sizeof(char));
                memcpy(&header_size, data + 1, sizeof(unsigned short));
                memcpy(&version, data + 3, sizeof(unsigned short));
                memcpy(&nr_sections, data + 5, sizeof(unsigned char));

                SFile sfile;
                for (int i = 0; i < nr_sections; i++) {
                    memcpy(&sfile, data + 6 + i * sizeof(SFile), sizeof(SFile));
                    if (i == section_no - 1) {
                        offset_sect = sfile.offset;
                        size_sect=sfile.size;
                    }
                }
                
                if (section_no < 1 || section_no > nr_sections) error = -1;
                if (offset + no_of_bytes > size_sect) error=-1;

                if (error == -1) {
                    write(fd_resp, "READ_FROM_FILE_SECTION#ERROR#", strlen("READ_FROM_FILE_SECTION#ERROR#")*sizeof(char));
                    continue;
                } else {
                    memcpy(shm_addr, data + offset_sect + offset, no_of_bytes);
                    write(fd_resp, "READ_FROM_FILE_SECTION#SUCCESS#", strlen("READ_FROM_FILE_SECTION#SUCCESS#")*sizeof(char));
                }


            }
            else if(strcmp(name, "READ_FROM_LOGICAL_SPACE_OFFSET#") == 0){

                unsigned int logical_offset = 0, no_of_bytes = 0;
                char magic='0',nr_sections='0';
                short int header_size=0,version=0;
                int error=1;
                read(fd_req, &logical_offset, sizeof(unsigned int));
                read(fd_req, &no_of_bytes, sizeof(unsigned int));


                memcpy(&magic, data, sizeof(char));
                memcpy(&header_size, data + 1, sizeof(unsigned short));
                memcpy(&version, data + 3, sizeof(unsigned short));
                memcpy(&nr_sections, data + 5, sizeof(unsigned char));

                
                if (logical_offset < 0 || no_of_bytes < 0 || logical_offset + no_of_bytes > shm_size) error=-1;


                if (error==-1) {
                    write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET#ERROR#", strlen("READ_FROM_LOGICAL_SPACE_OFFSET#ERROR#")*sizeof(char));
                    continue;

                } else {
                    memcpy(shm_addr, data, no_of_bytes);
                    write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET#SUCCESS#", strlen("READ_FROM_LOGICAL_SPACE_OFFSET#SUCCESS#")*sizeof(char));
                }


            }else {
               write(fd_resp, "UNKNOWN_REQUEST#ERROR#", strlen("UNKNOWN_REQUEST#ERROR#")*sizeof(char));
               return 1;
            }

            if (strcmp(name, "EXIT#") == 0) {
                break;
            }
        
        }
    }

    close(fd_req);
    close(fd_resp);
    unlink(RESP_PIPE);
    munmap(shm_addr, shm_size);
    shm_unlink(SHM_NAME);

    return 0;
}
