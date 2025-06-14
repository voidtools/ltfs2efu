
//
// Copyright (C) 2025 voidtools / David Carpenter
// 
// Permission is hereby granted, free of charge, 
// to any person obtaining a copy of this software 
// and associated documentation files (the "Software"), 
// to deal in the Software without restriction, 
// including without limitation the rights to use, 
// copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit 
// persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#pragma warning(disable : 4996)

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "version.h"

void ltfs2efu_write_string(HANDLE out_handle,char *s)
{
	int len;
	DWORD numwritten;
	
	len = (int)strlen(s);
	
	if (!WriteFile(out_handle,s,len,&numwritten,NULL))
	{
		printf("Error %u: Unable to write to out file.\n",GetLastError());
		
		ExitProcess(4);
	}

	if (len != numwritten)
	{
		printf("Error: Unable to write to out file.\n");
		
		ExitProcess(5);
	}
}

// short writes only
void ltfs2efu_write_printf(HANDLE out_handle,char *format,...)
{
	va_list args;
	char buf[1024];
	
	va_start(args,format);
	vsprintf(buf,format,args);
	va_end(args);
	
	ltfs2efu_write_string(out_handle,buf);
}

char *ltfs2efu_match(char *s,char *search)
{
	char *p1;
	char *p2;
	
	p1 = s;
	p2 = search;
	
	while(*p2)
	{
		if (!*p1)
		{
			return NULL;
		}
		
		if (*p1 != *p2)
		{
			return NULL;
		}
		
		p1++;
		p2++;
	}
	
	return p1;
}

char *ltfs2efu_expect(char *s,char *search)
{
	char *p;
	
	p = ltfs2efu_match(s,search);
	if (!p)
	{
		char gotbuf[256];
		int run;
		char *d;
		
		run = 255;
		d = gotbuf;
		p = s;
		
		while(run)
		{
			if ((*p == '\r') && (p[1] == '\n'))
			{
				break;
			}
			
			if (*p == '\n')
			{
				break;
			}
			
			if (!*p)
			{
				break;
			}
			
			*d++ = *p;
			
			p++;
			run--;
		}
		
		*d = 0;
		
		printf("Error: Expected %s got %s\n",search,gotbuf);
		
		ExitProcess(9);
	}
	
	return p;
}

// we have already removed the ending >
char *ltfs2efu_match_tag(char *s,char *search)
{
	char *p;
	
	p = ltfs2efu_match(s,search);
	if (p)
	{
		char *match_p;

		if (!*p)
		{
			return p;
		}

		match_p = ltfs2efu_match(p," ");
		if (match_p)
		{
			return match_p;
		}
	}
	
	return NULL;
}

char *ltfs2efu_skip(char *s,char *search)
{
	char *p;
	
	p = ltfs2efu_match(s,search);
	if (p)
	{
		return p;
	}
	
	return s;
}

char *ltfs2efu_find(char *s,char *search)
{
	char *p;
	
	p = s;
	
	while(*p)
	{
		if (ltfs2efu_match(p,search))
		{
			return p;
		}
	
		p++;
	}
	
	return NULL;
}

char *ltfs2efu_find_expect(char *s,char *search)
{
	char *p;
	
	p = ltfs2efu_find(s,search);
	if (!p)
	{
		printf("Error: Unable to find %s\n",search);
		
		ExitProcess(9);
	}
	
	return p;
}

void *ltfs2efu_alloc(SIZE_T size)
{
	void *p;
	
	p = malloc(size);
	if (!p)
	{
		printf("Error: Unable to alloc %Id\n",size);
		
		ExitProcess(10);
	}
	
	return p;
}

char *ltfs2efu_strcpy(char *buf,const char *s)
{
	char *d;
	const char *p;
	
	d = buf;
	p = s;
	
	while(*p)
	{
		*d++ = *p;
		
		p++;
	}
	
	*d = 0;
	
	return d;
}

char *ltfs2efu_stralloc_pathcombine(char *path,char *subfolder)
{
	size_t len;
	char *p;
	char *d;
	
	len = strlen(path) + 1 + strlen(subfolder);
	
	p = ltfs2efu_alloc(len + 1);
	
	d = p;
	if (*path)
	{
		d = ltfs2efu_strcpy(d,path);
		d = ltfs2efu_strcpy(d,"\\");
	}
	d = ltfs2efu_strcpy(d,subfolder);
	
	return p;
}

void ltfs2efu_fixtag(char *s)
{	
	char *d;
	char *p;
	
	d = s;
	p = s;
	
	while(*p)
	{
		char *match_p;
		
		match_p = ltfs2efu_match(p,"&lt;");
		if (match_p)
		{
			*d++ = '<';
			
			p = match_p;
			
			continue;
		}
		
		match_p = ltfs2efu_match(p,"&gt;");
		if (match_p)
		{
			*d++ = '>';
			
			p = match_p;
			
			continue;
		}
		
		match_p = ltfs2efu_match(p,"&amp;");
		if (match_p)
		{
			*d++ = '&';
			
			p = match_p;
			
			continue;
		}
		
		*d++ = *p;
	
		p++;
	}
	
	*d = 0;
}

char *ltfs2efu_file(HANDLE out_handle,char *s,char *path)
{
	char *p;
	char *name;
	char *length;
	char *date_created;
	char *date_modified;
	DWORD attributes;
	
	p = s;
	name = NULL;
	length = "";
	date_created = "";
	date_modified = "";
	attributes = 0;
	
	while(*p)
	{
		char *tagend;
		
		p = ltfs2efu_find(p,"<");
		if (!p)
		{
			break;
		}
		
		p++;
		
		tagend = ltfs2efu_find_expect(p,">");
		*tagend++ = 0;
		
//printf("FILE TAG %s\n",p);
		
		if (ltfs2efu_match_tag(p,"name"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			name = p;
				
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"readonly"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			
			if (ltfs2efu_match(p,"true"))
			{
				attributes |= FILE_ATTRIBUTE_READONLY;
			}
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"length"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;

			ltfs2efu_fixtag(p);
			length = p;
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"creationtime"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			date_created = p;
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"modifytime"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			date_modified = p;
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"/file"))
		{
			p = tagend;
			
			break;
		}
		else
		{
			p = tagend;
		}
	}
	
	if (name)
	{	
		char *subpath;
		
		subpath = ltfs2efu_stralloc_pathcombine(path,name);

		// ltfs2efu_write_string(out_handle,"Filename,Size,Date Modified,Date Created,Attributes\r\n");
		ltfs2efu_write_string(out_handle,"\"");
		ltfs2efu_write_string(out_handle,subpath);
		ltfs2efu_write_string(out_handle,"\"");
		ltfs2efu_write_string(out_handle,",");
		// Size
		ltfs2efu_write_string(out_handle,length);
		ltfs2efu_write_string(out_handle,",");
		// Date Modified
		ltfs2efu_write_string(out_handle,date_modified);
		ltfs2efu_write_string(out_handle,",");
		// Date Created
		ltfs2efu_write_string(out_handle,date_created);
		ltfs2efu_write_string(out_handle,",");
		// Attributes
		ltfs2efu_write_printf(out_handle,"%d",attributes);
		ltfs2efu_write_string(out_handle,"\r\n");			
				
		free(subpath);
	}
	
	return p;		
}

char *ltfs2efu_directory(HANDLE out_handle,char *s,char *path)
{
	char *p;
	char *name;
	char *subpath;
	DWORD attributes;
	char *date_created;
	char *date_modified;
	
	if (*path)
	{
		printf("DIRECTORY %s\n",path);
	}
	
	p = s;
	name = NULL;
	subpath = path;
	attributes = FILE_ATTRIBUTE_DIRECTORY;
	date_created = "";
	date_modified = "";
	
	while(*p)
	{
		char *tagend;
		
		p = ltfs2efu_find(p,"<");
		if (!p)
		{
			break;
		}
		
		p++;
		
		tagend = ltfs2efu_find_expect(p,">");
		*tagend++ = 0;
		
//printf("DIRECTORY TAG %s\n",p);

		if (ltfs2efu_match_tag(p,"name"))
		{
			p = tagend;
			
			if (subpath != path)
			{
				free(subpath);
			}
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			name = p;
			
			subpath = ltfs2efu_stralloc_pathcombine(path,name);
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"readonly"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);

			if (ltfs2efu_match(p,"true"))
			{
				attributes |= FILE_ATTRIBUTE_READONLY;
			}
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"creationtime"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			date_created = p;
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"modifytime"))
		{
			p = tagend;
			
			tagend = ltfs2efu_find_expect(p,"<");
			*tagend++ = 0;
			
			ltfs2efu_fixtag(p);
			date_modified = p;
			
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"directory"))
		{
			p = ltfs2efu_directory(out_handle,tagend,subpath);
			if (!p)
			{
				break;
			}
		}
		else
		if (ltfs2efu_match_tag(p,"contents"))
		{
			if (name)
			{
				// ltfs2efu_write_string(out_handle,"Filename,Size,Date Modified,Date Created,Attributes\r\n");
				ltfs2efu_write_string(out_handle,"\"");
				ltfs2efu_write_string(out_handle,subpath);
				ltfs2efu_write_string(out_handle,"\"");
				ltfs2efu_write_string(out_handle,",");
				// Size
				ltfs2efu_write_string(out_handle,",");
				// Date Modified
				ltfs2efu_write_string(out_handle,date_modified);
				ltfs2efu_write_string(out_handle,",");
				// Date Created
				ltfs2efu_write_string(out_handle,date_created);
				ltfs2efu_write_string(out_handle,",");
				// Attributes
				ltfs2efu_write_printf(out_handle,"%d",attributes);
				ltfs2efu_write_string(out_handle,"\r\n");
			}
			else
			{
				printf("Warning: contents with no name in %s\n",path);
			}
							
			p = tagend;
		}
		else
		if (ltfs2efu_match_tag(p,"file"))
		{
			p = ltfs2efu_file(out_handle,tagend,subpath);
			if (!p)
			{
				printf("Warning: missing </file> in %s\n",path);
				break;
			}
		}
		else
		if (ltfs2efu_match_tag(p,"/directory"))
		{
			p = tagend;
			
			break;
		}
		else
		{
			p = tagend;
		}
	}
		
	if (subpath != path)
	{
		free(subpath);
	}
	
	return p;							
}

// main entry
int main(int argc,char **argv)
{
	int ret;
	int wargv;
	wchar_t **wargc;
	
	printf("ltfs2efu " VERSION_TEXT "\n");
	
	ret = 0;
	
	wargc = CommandLineToArgvW(GetCommandLineW(),&wargv);
	
	wargv--;
	
	if (wargv != 2)
	{
		printf("Usage:\n");
		printf("ltfs2efu.exe <in.xml> <out.efu>\n");

		ret = 1;
	}
	else
	{
		HANDLE in_handle;
		
		in_handle = CreateFileW(wargc[1],GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,0,NULL);
		if (in_handle != INVALID_HANDLE_VALUE)
		{
			DWORD size;
			DWORD size_hi;
			
			size = GetFileSize(in_handle,&size_hi);
			
			if ((size != INVALID_FILE_SIZE) && (!size_hi))
			{
				char *buf;
				HANDLE out_handle;
				DWORD numread;
				
				buf = ltfs2efu_alloc(size + 1);
					
				if (ReadFile(in_handle,buf,size,&numread,NULL))
				{
					if (numread == size)
					{
						buf[size] = 0;
							
						out_handle = CreateFile(wargc[2],GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
						if (out_handle != INVALID_HANDLE_VALUE)
						{
							char *p;
							char *tagend;
							
							ltfs2efu_write_string(out_handle,"Filename,Size,Date Modified,Date Created,Attributes\r\n");
							
							p = buf;
							
							// bom
							p = ltfs2efu_skip(p,"\xEF\xBB\xBF");
							
							p = ltfs2efu_expect(p,"<?xml");
							tagend = ltfs2efu_find_expect(p,">");
							
							*tagend++ = 0;
							
							{
								char *encoding;
								
								encoding = ltfs2efu_find_expect(p," encoding=\"UTF-8\"");
							}
							
							ltfs2efu_directory(out_handle,tagend,"");
							
							CloseHandle(out_handle);
						}
						else
						{
							printf("Error %u: Unable to create %S\n",GetLastError(),wargc[2]);
						
							ret = 3;
						}
					}
					else
					{
						printf("Error: Unable to read in file.\n",GetLastError());
						
						ret = 8;
					}
				}
				else
				{
					printf("Error %u: Unable to read in file.\n",GetLastError());
					
					ret = 7;
				}
					
				free(buf);
			}
			else
			{
				printf("Error: in file too large\n");
				
				ret = 6;
			}
			
			CloseHandle(in_handle);
		}
		else
		{
			printf("Error %u: Unable to open %S\n",GetLastError(),wargc[1]);
			
			ret = 2;
		}
	}
	
	return ret;
}
