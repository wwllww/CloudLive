#include "BaseAfx.h"

static CreateOBJ g_DCreateOBJ[1024] = {0};
static int G_Num = 0;
int AddCreateOBject2Array(const char * ClassName,CREATEOBJ pf)
{
	if(0 == G_Num)
	{
		g_DCreateOBJ[0].Class_Name = ClassName;
		g_DCreateOBJ[0].fp = pf;
		++G_Num;
	}
	else
	{
		
		for(int i = 0; i < G_Num; ++i)
		{
			int ret = strcmp(ClassName,g_DCreateOBJ[i].Class_Name);
			if(ret > 0)
			{
				if(i == G_Num - 1)
				{
					g_DCreateOBJ[i + 1].Class_Name = ClassName;
					g_DCreateOBJ[i + 1].fp = pf;
					++G_Num;
					break;
				}
		
				else
					continue;
			}
			else if(ret < 0)
			{
				for(int j = G_Num - 1;j >= i; --j)	
				{
					g_DCreateOBJ[j + 1] = g_DCreateOBJ[j];
				}
				g_DCreateOBJ[i].Class_Name = ClassName;
				g_DCreateOBJ[i].fp = pf;
				++G_Num;
				break;
			}
			else
			{
				g_DCreateOBJ[i].fp = pf;
				break;
			}
		}
		
	}

	return G_Num;
}

BaseProcess* CREATEOBJECT(const char *Classname)
 {
    if( 0 == G_Num || !Classname)
		return NULL;
	int l = 0;
	int h = G_Num - 1;
	while(l <= h)
	{
		int m = (l + h) >> 1;
		int ret = strcmp(Classname,g_DCreateOBJ[m].Class_Name);
		
		if(ret > 0)
		{
			l = m + 1;
		}
		else if(ret < 0)
		{
			h = m - 1;
		}
		else 
		{
			return g_DCreateOBJ[m].fp();
		}
	}
	return NULL;
 }
