/*
	database.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <string.h>
#include "error.h"
#include "database.h"


void database::init()
{
	for(int i=0;i<1024;i++)
		bcks[i]=NULL;
	ostr=NULL;
	ostk=NULL;
	owrt=NULL;
}

void database::openreader(FILE* dbf)
{
	long pos; //Current position in file
	int lr; //Length read from file
	char* fnd; //String finding pointer
	FILE* strm; //Open filestream
	char onam[65]; //Object name found
	long opos; //Object position found
	char tmp[2049]; //Temporary reading buffer
	reader* rstk; //Reader to put on the stack

	strm=dbf;
	if(!strm)
		throw error("Error opening reader");

	tmp[0]='\0';
	lr=fread(tmp,1,strlen(MAGIC),strm);
	tmp[lr]='\0';
	if(strcmp(tmp,MAGIC)!=0)
		throw error("File is corrupt, or incompatible with this version");
	rstk=new reader;
	rstk->strm=strm;
	rstk->next=NULL;
	if(ostk)
		rstk->next=ostk;
	ostk=rstk;
	lr=2;
	opos=-1;
	while(lr>1)
	{
		pos=ftell(strm);
		lr=fread(tmp,1,256,strm);
		tmp[lr]='\0';
		if(tmp[0]=='\n' && tmp[1]=='@')
		{
			pos++;
			fseek(strm,pos,SEEK_SET);
			lr=fread(tmp,1,66,strm);
			tmp[lr]='\0';
			fnd=strstr(tmp,"\n");
			if(fnd)
			{
				if(opos!=-1)
					submitobj(strm,onam,opos,pos-opos);
				*fnd='\0';
				sprintf(onam,"%s",tmp+1);
				opos=pos;
			}
			fseek(strm,pos,SEEK_SET);
		}
		else
		{
			fnd=strstr(tmp,"\n@");
			if(fnd)
				fseek(strm,fnd-tmp+pos,SEEK_SET);
			else
				fseek(strm,-1,SEEK_CUR);
		}
	}
	if(opos!=-1)
		submitobj(strm,onam,opos,ftell(strm)-opos);
}

void database::openwriter(FILE* dbf) //Open a writer into the file at the given path
{
	closewriter();
	owrt=dbf;
	if(!owrt)
		throw error("Error opening writer");
	fprintf(owrt,"%s\n",MAGIC);
}

void database::closereader() //Close the last opened reader
{
	FILE* strm; //Stream to close
	reader* del; //Reader to delete
	obj* curr;
	obj* next; //Objects to inspect

	if(ostk)
	{
		del=ostk;
		strm=del->strm;
		fclose(strm);
		ostk=del->next;
		delete del;
		for(int i=0;i<1024;i++)
		{
			curr=bcks[i];
			while(curr)
			{
				if(curr->strm==strm)
				{
					if(curr->next)
						curr->next->prev=curr->prev;
					next=curr->next;
					if(curr->prev)
						curr->prev->next=curr->next;
					else
						bcks[i]=curr->next;
					delete[] curr->nam;
					delete curr;
					curr=next;
				}
				else
					curr=curr->next;
			}
		}
	}
}

void database::closewriter()
{
	if(owrt)
	{
		fclose(owrt);
		owrt=NULL;
	}
}

void database::switchobj(char* nam)
{
	obj* got; //Object got from database

	got=locateobj(nam);
	if(got)
	{
		if(ostr)
			delete[] ostr;
		ostr=new char[got->len+1];
		fseek(got->strm,got->pos,SEEK_SET);
		ostr[fread(ostr,1,got->len,got->strm)]='\0';
	}
	else
	{
		if(ostr)
			delete[] ostr;
		ostr=NULL;
		throw error("Object not found in database");
	}
}

char* database::getvalue(char* key,char* val)
{
	char srch[68]; //Key statement to search for	
	char* fnd; //Pointer to found string
	int lk; //Length of key
	char* out; //Value to return

	out=val;
	lk=strlen(key);
	if(lk>64)
		return NULL;
	if(ostr)
	{
		sprintf(srch,"\n%s =",key);
		fnd=strstr(ostr,srch);
		if(!fnd)
		{
			sprintf(srch,"\n%s=",key);
			fnd=strstr(ostr,srch);
		}
		if(fnd)
		{
			fnd+=strlen(srch);
			while(*fnd==' ')
				fnd++;
			for(int i=0;i<64 && *fnd!='\0' && *fnd!='\n';i++)
			{
				*val=*fnd;
				val++;
				fnd++;
			}
			*val='\0';
		}
		else
			out[0]='\0';
	}
	else
		out[0]='\0';
	return out;
}

long database::getvalue(char* key)
{
	char val[65]; //String representation
	long out; //Value to output

	getvalue(key,val);
	out=-1;
	sscanf(val,"%ld",&out);
	return out;
}

void database::putobject(char* nam)
{
	if(owrt)
		fprintf(owrt,"@%s\n",nam);
}

void database::putvalue(char* key,char* val)
{
	if(owrt)
		fprintf(owrt,"%s=%s\n",key,val);
}

void database::putvalue(char* key,long val)
{
	if(owrt)
		fprintf(owrt,"%s=%ld\n",key,val);
}

void database::submitobj(FILE* strm,char* nam,long pos,long len)
{
	int hash; //Hash of name
	obj* next; //Next bucket in chain to remember

	hash=hashstring(nam);
	if(bcks[hash])
		next=bcks[hash];
	else
		next=NULL;
	bcks[hash]=new obj;
	bcks[hash]->nam=new char[strlen(nam)+1];
	sprintf(bcks[hash]->nam,"%s",nam);
	bcks[hash]->strm=strm;
	bcks[hash]->pos=pos;
	bcks[hash]->len=len;
	bcks[hash]->next=next;
	if(next)
		next->prev=bcks[hash];
	bcks[hash]->prev=NULL;
}

obj* database::locateobj(char* nam)
{
	obj* next; //Next object to try
	int hash; //Hash of name

	hash=hashstring(nam);
	next=bcks[hash];
	while(next)
	{
		if(strcmp(nam,next->nam)==0)
			return next;
		next=next->next;
	}
	return NULL;
}

int database::hashstring(char* str)
{
	long out; //Value to output

	out=1;
	for(int i=0,j=strlen(str);i<j;i++)
		out=(out*str[i])%1024;
	return out;
}

obj* database::bcks[1024];
char* database::ostr;
reader* database::ostk;
FILE* database::owrt;
