#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

bool haveDuplicate = true;
void CreateArchive(const char* name, char **content);
void StoreContent(const char* name, FILE* fout);
void WriteDirectory(const char* name, FILE* fout);
void WriteFile(const char* name, FILE* fout);
void ExtractArchive(const char* name);
void ShowArchive(const char* name);
void ExtractFile(FILE *fin, const char *name, uint64_t sizeFile);
void ShowError(const char *msg);
int CheckDuplicate(const char* prevArchive, char* name);
void Menu();
void CheckFileDuplicate(const char* name, char* newName);
void CheckDirectoryDuplicate(const char* prevArchive, const char* name);

const char archiveFile[8] = "archive";

int main(int argc, char **argv)
{	
    if (argc < 3)
    {
    	Menu(argv[0]);
    }
    if (strcmp(argv[1], "cr") == 0)
    {
   	for(int i = 2; i < argc; i++)
   	{
   		CheckDuplicate(archiveFile, *(argv+i));
	}
        if(haveDuplicate == false)
        {
        	CreateArchive(archiveFile, argv + 2);
    	}else 
    	{	
    		printf("Archive file already exists.\n");
    	}
    }
    else if (strcmp(argv[1], "ex") == 0)
    {
        ExtractArchive(archiveFile);
    }
    else if (strcmp(argv[1], "ls") == 0)
    {
        ShowArchive(archiveFile);
    }
    return 0;
	
}

void CreateArchive(const char* name, char **content)
{
	// create archive file and check error 
	FILE* fout = fopen(name, "wb");
	if(!fout) 
	{
		ShowError("function CreateArchive()");
	}
	// store content

	for(; *content; ++content)
	{
		StoreContent(*content, fout);
	}
	// close file stream
	fclose(fout);		
}

void StoreContent(const char* name, FILE* fout)
{
	struct stat statBuffer;
    lstat(name, &statBuffer);
    
    bool is_dir = false;
    if(S_ISDIR(statBuffer.st_mode))
    {
    	is_dir = true;
    }

    uint32_t sizeFileName = strlen(name) + is_dir;
    fwrite(&sizeFileName, sizeof sizeFileName, 1, fout);
    fwrite(name, 1, sizeFileName - is_dir, fout);
    uint64_t sizeFile = statBuffer.st_size;
    
    if (is_dir)
    {
        fputc('/', fout); 
        sizeFile = 0;         // dirs have a "size" of 0
    }
    fwrite(&sizeFile, sizeof sizeFile, 1, fout);
    if (is_dir)
	WriteDirectory(name, fout);
    else
    	WriteFile(name, fout);
}

void WriteDirectory(const char* name, FILE* fout)
{		
	DIR* dir;
	struct dirent* entry;
	struct stat statBuffer;
	char n[1000000];
	
	if((dir = opendir(name)) == NULL) 
	{
		ShowError("function WriteDirectory()");
	}

		    	
	while((entry = readdir(dir)) != NULL)
	{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 	
           				{continue;}
           
		    	strcpy(n, name);
      			strcat(n, "/");
        		strcat(n, entry->d_name);
           		StoreContent(n, fout);
		
	}
	closedir(dir);	
}

void WriteFile(const char* name, FILE* fout)
{
	char buf[BUFSIZ];
    	FILE *fin = fopen(name, "rb");
    	
    	if (!fin) 
    	{
    		ShowError("function WriteFile()");
    	}
    
    	for (size_t n; (n = fread(buf, 1, sizeof buf, fin)) > 0; )
		fwrite(buf, 1, n, fout);
		
   	fclose(fin);
}

void ExtractArchive(const char* name)
{
	FILE *fin = fopen(name, "rb");
    	if (!fin) 
    	{
    		ShowError("function ExtractArchive()");
    	}
   	 char header[1000000];
    	for (int sizeFileName; fread(&sizeFileName, sizeof sizeFileName, 1, fin) > 0; )
    	{	
      		fread(header, 1, sizeFileName, fin);
       		header[sizeFileName] = '\0';
       		off_t sizeFile;
        	fread(&sizeFile, sizeof sizeFile, 1, fin);
        	if (header[strlen(header) - 1] == '/')
        	{
        		mkdir(header, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        	}
        	else
            	{
            		ExtractFile(fin, header, sizeFile);
    		}
   	}
    	fclose(fin);
}

void ExtractFile(FILE *fin, const char *name, uint64_t sizeFile)
{
    FILE *fout = fopen(name, "wb");
    if (!fout) 
    {
    	ShowError(name);
    }
    char buf[BUFSIZ];
    const uint64_t sizeBuffer = sizeof buf;
    for (uint64_t n;
         (n = fread(buf, 1, sizeFile > sizeBuffer ? sizeBuffer : sizeFile, fin)) > 0;
         sizeFile -= n)
        fwrite(buf, 1, n, fout);
    fclose(fout);
}

void ShowArchive(const char* name)
{
	FILE* fin = fopen(name, "rb");
	if(!fin)
	{
    		ShowError("ShowArchive");
    	}
	char header[1000000];
	for(int sizeFileName; fread(&sizeFileName, sizeof sizeFileName, 1, fin) > 0;)
	{
		fread(header, 1, sizeFileName, fin);
		header[sizeFileName] = '\0';
		printf("%s\n", header);
		off_t sizeFile;
		fread(&sizeFile, sizeof sizeFile, 1, fin);
		fseek(fin, sizeFile, SEEK_CUR);
	}
	fclose(fin);
}

void ShowError(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int CheckDuplicate(const char* prevArchive, char* name)
{

	struct stat statBuffer;
	lstat(name, &statBuffer);
	DIR* dir;
	struct dirent* entry;
	// open to read file archive, if it already exists;
	FILE* finPrev = fopen(prevArchive, "rb");
	if(finPrev == NULL)
	{
		finPrev = fopen(prevArchive, "ab+");
	} 
	
	char header[1000000] = {};
	off_t sizeFile = 0;
	char bufPrev[BUFSIZ] = {};
	int sizeBufPrev = sizeof bufPrev;
	
	int count = 0;
	// search for name of file being archived in existing archive file
	for (uint32_t sizeFileName; fread(&sizeFileName, sizeof sizeFileName, 1, finPrev) > 0; )
    	{	
    		// read file's header
      		fread(header, 1, sizeFileNamelms, finPrev);
       		header[sizeFileName] = '\0';
       		// read size of file
		fread(&sizeFile, sizeof sizeFile, 1, finPrev);
		char buf[BUFSIZ] = {};
   		int sizeBuf = sizeof buf;
   		// read content if it's a file
   		// "sizeFile" <= 0 if it"s a directory
		if(sizeFile > 0)	
			fread(buf, 1, sizeFile > sizeBuf ? sizeBuf : sizeFile, finPrev);
		strcpy(bufPrev, buf);
					
		// if found 
		// then stop searching
		if (strstr(header, name))
			break;	
		if (feof(finPrev) == 0)
			break;
	} // end for
			// if current file is a directory
			if(S_ISDIR(statBuffer.st_mode))		
			{ 	
				
				CheckDirectoryDuplicate(prevArchive, name);		
			} // end if
			else
			{	
				CheckFileDuplicate(name, bufPrev);		
			} // end else
		
	
	fclose(finPrev);
	return haveDuplicate;

	
}

void CheckDirectoryDuplicate(const char* prevArchive, const char* name)
{
	DIR* dir;
	struct stat staBuffer;
	struct dirent* entry;
	// name of content(file or another directory) 
	// inside current directory
	char n[10000] = {};
	if((dir = opendir(name)) == NULL) 
	{
		ShowError("function CheckDirectoryDuplicate()");
	}
	while ((entry = readdir(dir)) != NULL)
	{
		// ignoring ".." and "."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 	
			{continue;}
		// recursive checking for deeper content in this directory		
		strcpy(n, name);
		strcat(n, "/");
		strcat(n, entry->d_name);
   		CheckDuplicate(prevArchive, n);				
	} // end while
	closedir(dir);
}

void CheckFileDuplicate(const char* name, char* bufPrev)
{	
	FILE* fin = fopen(name, "rb");
	if(fin == NULL)
	{
		ShowError("function CheckFileDuplicate()");
	} 
	struct stat statBuffer;
	lstat(name, &statBuffer);
	//size of file being archived
	uint64_t size = statBuffer.st_size;
	
	char bufNew[BUFSIZ] = {};
	int sizeBufNew = sizeof bufNew;
	// read content of file (which is being archived) into "bufNew"
	fread(bufNew, 1, sizeof bufNew, fin);
	// if there is one or more file which is 
	// different from previous archive,
	// then allows to make new archive
	// otherwise new archive files are considered to 
	// be the same as in previous archive
	// "haveDuplicate" always set as true
	if (strcmp(bufPrev, bufNew) != 0)
	{
		haveDuplicate = false; 
	}
	fclose(fin);
}
void Menu()
{
    fprintf(stderr, "Command:\n	./[Executable] cr [File(Dir)ToArchive1] [File(Dir)ToArchive2]... -- Create archive.\n");
    fprintf(stderr, "       	./[Executable] ls [Name_ArchiveFile]\t\t-- List Content\n");	
    fprintf(stderr, "       	./[Executable] ex [Name_ArchiveFile]\t\t-- Extract archive\n");
    exit(EXIT_FAILURE);
}
