#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct __attribute__((packed)) {
    char name[11];
    short int type;
    int offset;
    int size;
} SFile;

void list(char *path, int recursive, char *name_ends_with, int size_greater) {
    
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char fullPath[1024];

    if ((dir = opendir(path)) == NULL) {
        perror("ERROR");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode))
                 {  
                    if (size_greater <= 0 || statbuf.st_size > size_greater) 
                    {
                        if (name_ends_with == NULL || strstr(entry->d_name, name_ends_with) != NULL) {
                         printf("%s\n", fullPath);
                        }
                    }
                    if (recursive) {
                        list(fullPath, recursive, name_ends_with, size_greater);
                    }
                } else if (S_ISREG(statbuf.st_mode)) {
                    if (size_greater <= 0 || statbuf.st_size > size_greater) {

                         if (name_ends_with == NULL || strstr(entry->d_name, name_ends_with) != NULL) {
                         printf("%s\n", fullPath);
                        }
                    }
                }
            }
        }
    }
    closedir(dir);
}

void sfile_print(char *name) {
    for (int i = 0; i < 11; i++) {
        if (name[i] == '\0') {
            break; 
        }
        printf("%c", name[i]); 
    }
}

int validate_sf_format(int fd) {

    char magic,nr_sections;
    short int header_size,version;
    int r1,r2,r3,r4;

    r1=read(fd, &magic, sizeof(char));
    r2=read(fd, &header_size, sizeof(short int ));
    r3=read(fd, &version, sizeof(unsigned short));
    r4=read(fd, &nr_sections, sizeof(unsigned char));

    if (r1==-1 || r2==-1 || r3==-1 || r4==-1) {
        printf("ERROR\nimpossible to read\n");
        return 0;
    }

    if (magic != '9') {
        printf("ERROR\nwrong magic\n");
        return 0;
    }

    if (version < 96 || version > 195) {
        printf("ERROR\nwrong version\n");
        return 0;
    }

    if (nr_sections != 2 && (nr_sections < 8 || nr_sections > 16)) {
        printf("ERROR\nwrong sect_nr\n");
        return 0;
    }

   
    for (int i = 0; i < nr_sections; i++) {

        SFile sfile;
        if (read(fd, &sfile, sizeof(SFile)) == -1) {
            printf("ERROR\nimpossible to read section header\n");
            return 0;
        }

        if (sfile.type != 85 && sfile.type != 17 && sfile.type != 89) {
            printf("ERROR\nwrong sect_types\n");
            return 0;
        }
    }

    
    printf("SUCCESS\nversion=%hu\nnr_sections=%hhu\n", version, nr_sections);
    lseek(fd, sizeof(char)*2 + sizeof(short int)*2, SEEK_SET);
    
    for (int i = 0; i < nr_sections; i++) {
        SFile sfile;
        if (read(fd, &sfile, sizeof(SFile))==-1) {
            printf("ERROR\nimpossible to read section header\n");
            return 0;
        }
        printf("section%d: ", i + 1);
        sfile_print(sfile.name);
        printf(" %hu %u\n", sfile.type, sfile.size);
    }

    return 1;
}

int validate_sf_format_v2(int fd) {

    char magic,nr_sections;
    short int header_size,version;
    int r1,r2,r3,r4;

    r1=read(fd, &magic, sizeof(char));
    r2=read(fd, &header_size, sizeof(short int ));
    r3=read(fd, &version, sizeof(short int ));
    r4=read(fd, &nr_sections, sizeof(char));

    if (r1==-1 || r2==-1 || r3==-1 || r4==-1) {
        return 0;
    }

    if (magic != '9') {
        return 0;
    }

    if (version < 96 || version > 195) {
        return 0;
    }

    if (nr_sections != 2 && (nr_sections < 8 || nr_sections > 16)) {
        return 0;
    }

   
    for (int i = 0; i < nr_sections; i++) {

        SFile sfile;
        if (read(fd, &sfile, sizeof(SFile)) == -1) {
            return 0;
        }
        if (sfile.type != 85 && sfile.type != 17 && sfile.type != 89) {
            return 0;
        }
    }

    lseek(fd, sizeof(char)*2 + sizeof(short int)*2, SEEK_SET);
    
    for (int i = 0; i < nr_sections; i++) {
        SFile sfile;
        if (read(fd, &sfile, sizeof(SFile))==-1) {
            return 0;
        }
    }
    return 1;
}

char* extract(int fd, int section_number, int line_number) {
    
    
    if (section_number < 1 || section_number > 16) {
        printf("ERROR\ninvalid section\n");
        return NULL;
    }

   
    if (line_number < 1) {
        printf("ERROR\ninvalid line\n");
        return NULL;
    }

    
    lseek(fd,sizeof(char)*5,SEEK_SET);
    int nr_section;
    read(fd, &nr_section, sizeof(char));


    off_t section_offset=0;
    int sec_size=0;
    for (int i = 1; i <= nr_section; i++) {
        SFile sfile;
        if (read(fd, &sfile, sizeof(SFile)) == -1) {
            printf("ERROR\nimpossible to read section header\n");
            return NULL;
        }

        if (i == section_number) {
            section_offset = sfile.offset;
            sec_size = sfile.size;
            break;
        }
    }
    
    if (lseek(fd, section_offset, SEEK_SET) == -1) {
        printf("ERROR\nimpossible to set file offset\n");
        return NULL;
    }

    
    char *buffer = (char *)malloc(sec_size* sizeof(char));
    if (buffer == NULL) {
        printf("ERROR\nimpossible to allocate memory\n");
        return NULL;
    }
    
    ssize_t nr_bytes;
    if ((nr_bytes = read(fd, buffer, sec_size)) == -1) {
        printf("ERROR\nimpossible to read from file\n");
        free(buffer);
        return NULL;
    }

    
    int line_count = 0;
    for (int i = nr_bytes - 1; i > 0; i--) {
        if (buffer[i] == '\n') {
            line_count++;
            if (line_count == line_number) {
                int start = i+1;
                int end = i;
                while (start < nr_bytes && buffer[start] != '\n') {
                    start++;
                }
                char *line = (char *)malloc((start - end) * sizeof(char)); 
                if (line == NULL) {
                    printf("ERROR\nimpossible to allocate memory\n");
                    free(buffer);
                    return NULL;
                }

                int j = 0;
                for (int i = end + 1; i < start; i++) {
                line[j] = buffer[i];
                j++;
                }
                line[j] = '\0';
                free(buffer);
                return line;
            }
        }
    }
    
    if (line_count < line_number) {
        printf("ERROR\nline not found\n");
    }
    free(buffer);
    return NULL;
}

int large_sect(int fd) {
     
    lseek(fd,sizeof(char)*5,SEEK_SET);
    int nr_section;
    read(fd, &nr_section, sizeof(char));
    for (int i = 1; i <= nr_section; i++) {
        SFile sfile;
        if (read(fd, &sfile, sizeof(SFile)) == -1) {
            printf("ERROR\nimpossible to read section header\n");
            return 1;
        }
        if (sfile.size > 1098) {
            return 1; 
        }
    }
  
    return 0; 
}

void findall(char *path, char *sf_files[], int *sf_count) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char fullPath[1024];

    if ((dir = opendir(path)) == NULL) {
        printf("ERROR\ninvalid directory path\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    findall(fullPath, sf_files, sf_count); 
                } else if (S_ISREG(statbuf.st_mode)) {
                    int fd = open(fullPath, O_RDONLY);
                    if (fd != -1) {
                        if (validate_sf_format_v2(fd)) {
                            struct stat file_stat;
                            if (fstat(fd, &file_stat) != -1) {
                                    if (large_sect(fd)==0) 
                                    {
                                        sf_files[*sf_count] = (char *)malloc(strlen(fullPath) + 1);
                                        if (sf_files[*sf_count] != NULL) {
                                        strcpy(sf_files[*sf_count], fullPath);
                                        (*sf_count)++;
                                        } else {
                                            printf("ERROR\nimpossible to allocate memory\n");
                                        }
                                    }
                                
                            }
                        }
                        close(fd);
                    }
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char **argv) {

    char *path = NULL;
    int size_greater = 0;
    char *name_ends_with = NULL;
    int recursive = 0;
    int sect_nr = 0;
    int line_nr = 0;
    char *sf_files[1024]; 
    int sf_count = 0;
    
    if (argc >= 2 && strcmp(argv[1], "parse") == 0) {
        for (int i = 2; i < argc; i++) {
            if (strstr(argv[i], "path=") != NULL && strstr(argv[i], "path=") == argv[i]) {
                path = strchr(argv[i], '=') + 1;
            }
        }
        if (path != NULL) {
            int fd = open(path, O_RDONLY);
            if (fd != -1) {
                validate_sf_format(fd);
                close(fd);
            } else {
                printf("ERROR\nimpossible to open file\n");
            }
        } else {
            printf("ERROR\nmissing file path\n");
        }
    } 


    else if (argc >= 2 && strcmp(argv[1], "variant") == 0) {
        printf("23302\n");
    } 


    else if (argc >= 2 && strcmp(argv[1], "list") == 0) {
        
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "recursive") == 0) {
                recursive = 1;
            } else if (strstr(argv[i], "path=") != NULL && strstr(argv[i], "path=") == argv[i]) {
                path = strchr(argv[i], '=') + 1;
            } else if (strstr(argv[i], "name_ends_with=") != NULL) {
                name_ends_with = strchr(argv[i], '=') + 1;
            } else if (strstr(argv[i], "size_greater=") != NULL) {
                size_greater = atoi(strchr(argv[i], '=') + 1);
            }
        }

        if (path != NULL) {
            printf("SUCCESS\n");
            list(path, recursive, name_ends_with, size_greater);
        } else {
            printf("ERROR\ninvalid directory path\n");
        }
    } 


    else if (argc >= 2 && strcmp(argv[1], "extract") == 0) {
    for (int i = 2; i < argc; i++) {
        if (strstr(argv[i], "path=") != NULL && strstr(argv[i], "path=") == argv[i]) {
            path = strchr(argv[i], '=') + 1;
        } else if (strstr(argv[i], "section=") != NULL && strstr(argv[i], "section=") == argv[i]) {
            sect_nr = atoi(strchr(argv[i], '=') + 1);
        } else if (strstr(argv[i], "line=") != NULL && strstr(argv[i], "line=") == argv[i]) {
            line_nr = atoi(strchr(argv[i], '=') + 1);
        }
    }
    if (path != NULL && sect_nr > 0 && line_nr > 0) {
        int fd = open(path, O_RDONLY);
        if (fd != -1) {
            char *line = extract(fd, sect_nr, line_nr);
            if (line != NULL) {
                printf("SUCCESS\n%s\n", line);
                free(line); 
                close(fd);
             } 
         }
         else {
            printf("ERROR\nimpossible to open file\n");
             }
         } 
    }

    else if (argc >= 2 && strcmp(argv[1], "findall") == 0) {
        for (int i = 2; i < argc; i++) {
            if (strstr(argv[i], "path=") != NULL && strstr(argv[i], "path=") == argv[i]) {
                path = strchr(argv[i], '=') + 1;
            }
        }
        if (path != NULL) {
            findall(path, sf_files, &sf_count);
            if (sf_count > 0) {
                printf("SUCCESS\n");
                for (int i = 0; i < sf_count; i++) {
                    printf("%s\n", sf_files[i]);
                }
                for (int i = 0; i < sf_count; i++) {
                free(sf_files[i]);
                }   

            } else {
                printf("SUCCESS\n");
            }
        } else {
            printf("ERROR\nmissing directory path\n");
        }
    } 

    else {
        printf("ERROR\ninvalid arguments\n");
    }

    return 0;
}