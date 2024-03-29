/*******************************************************************************
 *
 *   tablefun.cpp -- 
 *
 *   Bj�rn Nilsson, 2004-2008
 */

#include <stdio.h>
#include "tablefun.h"
#include "nan.h"
#include "stringfun.h"
#include "math/rand.h"
#include "math/statfun.h"
#include "types/minmax.h"
#include "system/linereader.h"

/******************************************************************************/
// Loading and parsing

bool Table_ScanFloat(const char *ach, float &x)
{
	if (ach[0]=='-')
	{
		if (ach[1]=='1' && ach[2]=='.' && ach[3]=='#')
		{
			if (ach[4]=='I' && ach[5]=='N' && ach[6]=='F')
			{
				x= Stat_GetNegInfty_float();
				return true;
			}
			else if (ach[4]=='Q' && ach[5]=='N' && ach[6]=='A' && ach[7]=='N')
			{
				x= Stat_GetNaN_float();
				return true;
			}
			x= Stat_GetNaN_float();
			return false;
		}
	}
	else
	{
		if (ach[0]=='1' && ach[1]=='.' && ach[2]=='#')
		{
			if (ach[3]=='I' && ach[4]=='N' && ach[5]=='F')
			{
				x= Stat_GetPosInfty_float();
				return true;
			}
			else if (ach[3]=='Q' && ach[4]=='N' && ach[5]=='A' && ach[6]=='N')
			{
				x= Stat_GetNaN_float();
				return true;
			}
			x= Stat_GetNaN_float();
			return false;
		}
	}

	if (sscanf(ach, "%f", &x)>0)
		return true;
	x= Stat_GetNaN_float();
	return false;
}


bool Table_ScanDouble(const char *ach, double &x)
{
	if (ach[0]=='-')
	{
		if (ach[1]=='1' && ach[2]=='.' && ach[3]=='#')
		{
			if (ach[4]=='I' && ach[5]=='N' && ach[6]=='F')
			{
				x= Stat_GetNegInfty_float();
				return true;
			}
			else if (ach[4]=='Q' && ach[5]=='N' && ach[6]=='A' && ach[7]=='N')
			{
				x= Stat_GetNaN_float();
				return true;
			}
			x= Stat_GetNaN_float();
			return false;
		}
	}
	else
	{
		if (ach[0]=='1' && ach[1]=='.' && ach[2]=='#')
		{
			if (ach[3]=='I' && ach[4]=='N' && ach[5]=='F')
			{
				x= Stat_GetPosInfty_float();
				return true;
			}
			else if (ach[3]=='Q' && ach[4]=='N' && ach[5]=='A' && ach[6]=='N')
			{
				x= Stat_GetNaN_float();
				return true;
			}
			x= Stat_GetNaN_float();
			return false;
		}
	}

	x= atof(ach); // TODO: error checking...
	return true; 
	//x= Stat_GetNaN_float();
	//return false;
}

const char *Table_CheckFloatFormat(const char *ach)
{
	// Check mantissa
	if (*ach=='-')
		ach++;
	const char *ach0= ach;
	while (*ach>='0' && *ach<='9')
		ach++;
	if (*ach=='.')
	{
		ach++;
		if (*ach=='#')
		{
			ach++;
			if (strncmp(ach, "INF",3)==0)
				return ach+3;
			if (strncmp(ach, "QNAN",4)==0)
				return ach+4;
		}
		else
		{
			ach0= ach;
			while (*ach>='0' && *ach<='9')
				ach++;
		}
	}
	if (ach0==ach)
		return NULL;

	// Check (optional) exponent
	if (*ach=='d' || *ach=='D' || *ach=='e' || *ach=='E')
	{
		ach++;
		if (*ach=='-' || *ach=='+')
			ach++;
		if (*ach>='0' && *ach<='9')
		{
			ach++;
			while (*ach>='0' && *ach<='9')
				ach++;
		}
		else
			return NULL;
	}

	// Return forwarded ptr
	return ach;
}

const char *Table_CheckFloatFormat_complete(const char *ach)
{
	// Check that the complete string is in float format
	ach= Table_CheckFloatFormat(ach);
	if (!ach || ach[0]!=0)
		return NULL;
	return ach;
}

const char *Table_CheckIntFormat(const char *ach)
{
	if (*ach=='-')
		ach++;
	const char *ach0= ach;
	while (*ach>='0' && *ach<='9')
		ach++;
	if (ach0==ach)
		return NULL;

	// Return forwarded ptr
	return ach;
}

const char *Table_CheckIntFormat_complete(const char *ach)
{
	// Check that the complete string is in float format
	ach= Table_CheckIntFormat(ach);
	if (!ach || ach[0]!=0)
		return NULL;
	return ach;
}

const char *Table_CheckUnsignedIntFormat(const char *ach)
{
	const char *ach0= ach;
	while (*ach>='0' && *ach<='9')
		ach++;
	if (ach0==ach)
		return NULL;

	// Return forwarded ptr
	return ach;
}

const char *Table_CheckUnsignedIntFormat_complete(const char *ach)
{
	// Check that the complete string is in float format
	ach= Table_CheckUnsignedIntFormat(ach);
	if (!ach || ach[0]!=0)
		return NULL;
	return ach;
}

const char *Table_SkipUnsignedInt(const char *ach)
{
	// Accepts empty cell in contrast to Table_Check
	while (*ach>='0' && *ach<='9')
		ach++;
	return ach;
}

bool Table_DetectAnnotation(const CTableIndex &ti, 
								int &nAnnotRows, 
								int &nAnnotCols, 
								CMatrix<short> &mFlags)
{
	// Auto-detects the number of annotation rows and columns.
	// As a side-effect it attempts to convert all elements 
	// to floats.

	// Input:	ti is a rectangular, delimited data table.
	//			nAnnotRows, nAnnotCols contains the minimum number 
	//			of annotation rows/columns in the datasets.
	// 
	// Output:	nAnnotRows, nAnnotCols will contain the autodetect 
	//			number of annotation rows/columns. Those values, 
	//			however, will always be larger than the bounds 
	//			given by the input values.
	//			mValues will contain float values (if parsed correctly)
	//			mFlags will contain parsing error flags.

	const int C=ti.GetMinColCount();
	const int R=ti.GetRowCount();
	if (C==0 || R==0)
	{
		nAnnotRows= 0;
		nAnnotCols= 0;
		return true;
	}

	try
	{
		mFlags.ReInit(R, C, 0);
	}
	catch(...)
	{
		// out of memory
		return false;
	}

	if (nAnnotRows>R)
		nAnnotRows= R;
	if (nAnnotCols>C)
		nAnnotCols= C;

	for (int r=0;r<R;r++)
		for (int c=0;c<C;c++)
		{
			const char *ach0= ti.GetFirstPointer(r, c);
			char *ach1= (char *)ti.GetLastPointer(r, c);
			
			if (ach0==ach1 || ((ach1-ach0==4) && strncmp("NULL", ach0, 4)==0))
				mFlags.SetAt(r, c, TF_Missing);
			else if (ach1!=Table_CheckFloatFormat(ach0))
				mFlags.SetAt(r, c, TF_Error);
		}

	if (nAnnotRows<0)
		nAnnotRows= 0;
	if (nAnnotCols<0)
		nAnnotCols= 0;

	/*
	// this is obsolete -- now replaced by pruning of pure int columns
	// Detect at least one annotation row/column
	if (R>1 && nAnnotRows<1)
		nAnnotRows= 1;
	if (C>1 && nAnnotCols<1)
		nAnnotCols= 1;
	*/

	if (R>0 && C>0 && (mFlags.GetAt(R-1,C-1) & TF_Error)==0)
	{
		int c_limit= nAnnotCols-1;
		int cmax= C-1;
		int rmax= R-1;
		int fmax= 0;

		// Select the rectangle that contains the largest no of valid floats		
		for (int r=R;--r>=nAnnotRows;)
		{
			int c=C-1;
			while (c>c_limit && (mFlags.GetAt(r,c) & TF_Error)==0)
				c--;
			if (c>c_limit)
				c_limit= c;

			int f= (C-1-c)*(R-r); // No. of valid floats within rect.
			if (f>fmax)
			{
				fmax= f;
				cmax= c;
				rmax= r;
			}

			int n= (C-1)-c; // Remaining no of error-free columns.
			if (n==0)
				break;
		}

		nAnnotRows= rmax;
		nAnnotCols= cmax+1;
	}
	else
	{
		nAnnotRows= R;
		nAnnotCols= C;
	}

	// Redeclare cols that lack proper floats AND are adjacent to the data as annotations
	// Motivation: 
	//	(a) Some annotations, e.g. LocusLinkID, are unsigned ints
	//	(b) Expression data are normally proper floats (with decimal point), rarely ints
	bool bInt= true;
	for (int c=nAnnotCols;bInt && c<C;c++)
	{
		if (nAnnotRows<R) // Must be content there to examine
		{
			for (int r=nAnnotRows;bInt && r<R;r++)
			{
				if (mFlags.GetAt(r,c)==0)
				{
					const char *ach= ti.GetFirstPointer(r,c);
					ach= Table_SkipUnsignedInt(ach);					
					if (ach==NULL || ach<ti.GetLastPointer(r,c))
						bInt= false;
				}
			}

			if (bInt)
				nAnnotCols= c+1;
		}
		else
			bInt= false;
	}

	bInt= true;
	for (int r=nAnnotRows;bInt && r<R;r++)
	{
		if (nAnnotCols<C) // Must be content there to examine
		{
			for (int c=nAnnotCols;bInt && c<C;c++)
			{
				const char *ach= ti.GetFirstPointer(r,c);
				ach= Table_SkipUnsignedInt(ach);
				if (ach==NULL || ach<ti.GetLastPointer(r,c))
					bInt= false;
			}

			if (bInt)
				nAnnotRows= r+1;
		}
		else
			bInt= false;
	}

	// If the entire matrix consists of annotations, we must 
	// decide if it should be interpreted as columns or rows.
	if (nAnnotRows==R || nAnnotCols==C)
	{
		if (R>C)
		{
			// If table is high and narrow, guess it contains named annotation columns
			nAnnotRows= 1;
			nAnnotCols= C;
		}
		else
		{
			// If table is low and wide, guess it contains named annotation rows
			nAnnotRows= R;
			nAnnotCols= 1;
		}
	}

	return true;
}

bool Table_ScanFloatMatrix(const CTableIndex &ti,
						  int row0, int col0,
						  int row1, int col1,
						  CMatrix<float> &mValues, 
						  CMatrix<short> *pFlags /* can be NULL */)
{
	// Attempts to convert a rectangular subset 
	// of a text table into a float values, *pValues.
	// Error codes will be reported in *pFlags.

	// Check subset range. 
	// Note that column range checking is done on a per-row basis below.
	if (row0<0 || col0<0 || row1<row0 || col1<col0 || row1>=ti.GetRowCount())
	{
		ASSERT(false);
		return false;
	}

	try
	{
		// Set matrix size.
		mValues.ReInit(row1-row0+1, col1-col0+1, 0);
		if (pFlags)
			pFlags->ReInit(row1-row0+1, col1-col0+1, 0);
	}
	catch(...)
	{
		// Out of memory.
		return false;
	}

	// Parse floats.
	const float fNaN= Stat_GetNaN_float();
	float *p, *p_end;
	mValues.GetFirstLastPtrs(&p, &p_end); // Sequential addressing increases speed.
	for (int r=row0;r<=row1;r++)
	{
		if (col1>=ti.GetColCount(r))
			return false; // Column out of bounds.

		for (int c=col0;c<=col1;c++)
		{
			const char *ach0= ti.GetFirstPointer(r, c);
			char *ach1= (char *)ti.GetLastPointer(r, c);
			
			if (ach0!=ach1 && !(ach1-ach0==4 && strncmp("NULL", ach0, 4)==0))
			{
				float value;
				char tmp= *ach1;
				*ach1= 0; // Temporary null terminator

				if (Table_ScanFloat(ach0, value))
					*p++= value;
				else
				{
					if (pFlags)
						pFlags->SetAt(r-row0, c-col0, TF_Error);
					*ach1= tmp;
					return false;
				}
				
				*ach1= tmp;
			}
			else
			{
				// Empty string indicates missing data
				if (pFlags)
					pFlags->SetAt(r-row0, c-col0, TF_Missing);
				*p++ = fNaN;
			}
		}
	}

	return true;
}

void Table_FindByFlags(const CMatrix<short> &mFlags, short mask, int &row, int &col)
{
	for (int r=0;r<mFlags.GetHeight();r++)
		for (int c=0;c<mFlags.GetWidth();c++)
			if ((mFlags(r, c) & mask)!=0)
			{
				row= r;
				col= c;
				return;
			}

	row= -1;
	col= -1;
}


CLoadBuf *Table_LoadTabDatasetHeader(CString sFilename, CTableIndex &ti, CString &sErr)
{
	CLineReader lr;

	CVector<char> v_buf(200000000); // 200 MB line buffer 
	CVector<const char *> v_items(20000000); // max 10 million columns
	int n_items;

	CLoadBuf *pNew= 0;
	if (lr.Open(sFilename))
	{
		pNew= g_FileSystem.AllocBuf();
		if (pNew)
		{
			if (lr.GetLine(v_buf.GetBuffer(), v_buf.GetSize(), v_items, n_items))
			{
				int sz= 0;
				for (int i=0;i<n_items;i++)
					sz += strlen(v_items[i])+1;
				pNew->SetSize(sz);

				CTableIndexRow tir;
				const char *ach0= v_items[0];
				char *ach1= pNew->m_aLoadBuf;
				for (int i=0;i<n_items;i++)
				{
					tir.m_vFirst.Add(ach1);
					while (*ach0)
						*ach1++ = *ach0++;
					tir.m_vLast.Add(ach1);
					*ach1++ = *ach0++;
				}
				ti.m_vRows.Add(tir);

				if (ach1-pNew->m_aLoadBuf!=sz)
				{
					sErr= "Internal error in Table_LoadTabDataset_header";
					g_FileSystem.FreeBuf(pNew);
					lr.Close();
					return 0;
				}
			}
		}
		else
			sErr= "Cannot allocate load buffer";

		lr.Close();
	}
	else
		sErr= "File not found";

	return pNew;
}

CLoadBuf *Table_LoadTabDataset(CString sFilename, CTableIndex &ti, CString &sErr)
{
	CString sExt= g_FileSystem.GetFileExtension(sFilename);
	if (!g_FileSystem.FileExists(sFilename) && sExt.IsEmpty())
		sFilename += ".txt";

	if (!g_FileSystem.FileExists(sFilename))
	{
		sErr= "File not found";
		return NULL;
	}

	CLoadBuf *pLB= g_FileSystem.Load(sFilename);
	if (!pLB)
	{
		sErr= "Unable to open file";
		return NULL;
	}
	if (ti.ScanTDS(pLB->m_aLoadBuf))
		return pLB;
	else
	{
		sErr= "Unable to parse dataset";
		g_FileSystem.FreeBuf(pLB);
		return NULL;
	}
}

bool Table_ScanTabDataset(const CTableIndex &idx, int &nAnnotRows, int &nAnnotCols, CMatrix<float> &mData)
{
	//
	// Autodetect number of annotation rows and columns (if not specified explicitly)
	// and parse floating point matrix body. 
	//
	CMatrix<short> mFlags;
	int nAutoRows= -1;
	int nAutoCols= -1;
	if (nAnnotRows<0 || nAnnotCols<0)
		if (Table_DetectAnnotation(idx, nAutoRows, nAutoCols, mFlags))
		{
			if (nAnnotRows<0)
				nAnnotRows= nAutoRows;
			if (nAnnotCols<0)
				nAnnotCols= nAutoCols;
			nAnnotRows= __min(idx.GetRowCount(), nAnnotRows);
			nAnnotCols= __min(idx.GetMinColCount(), nAnnotCols);
		}
		else
			return false;

	if (!Table_ScanFloatMatrix(idx, nAnnotRows, nAnnotCols, idx.GetRowCount()-1, idx.GetMinColCount()-1, mData, &mFlags))
		return false;

	return true;
}

/******************************************************************************/
// Sorting

bool Table_GetKeyColumn(const CTableIndex &ti, 
						int nColumn,
						int nAnnotRows,
						CVector<Table_SortItem> &vItems)
{
	// Extract key column.

	// Returns true if all keys are valid floats, false otherwise (too support 
	// choice between alphabetic and numeric sort).
	int R= ti.GetRowCount();
	vItems.SetSize(R-nAnnotRows);
	bool bOk= true;
	for (int r=nAnnotRows;r<R;r++)
	{
		const char *ach0= ti.GetFirstPointer(r, nColumn);
		const char *ach1= ti.GetLastPointer(r, nColumn);
		
		Table_SortItem i;
		i.m_nIndex= r;
		i.m_sKey= CString(ach0, ach1-ach0);
		if (bOk)
		{
			bOk= Table_CheckFloatFormat_complete(i.m_sKey)!=NULL;
			if (bOk)
				(void) Table_ScanFloat(i.m_sKey, i.m_fValue);
			else
				i.m_fValue= Stat_GetNaN_float();
		}
		else
			i.m_fValue= 0;

		vItems.SetAt(r-nAnnotRows, i);
	}

	return bOk;
}

bool Table_GetKeyRow(const CTableIndex &ti,
					 int nRow,
					 int nAnnotCols,
					 CVector<Table_SortItem> &vItems)
{
	// Extract key row

	// Returns true if all keys are valid floats, false otherwise (too support 
	// choice between alphabetic and numeric sort).
	int C= ti.GetMinColCount();
	vItems.SetSize(C-nAnnotCols);
	bool bOk= true;
	for (int c=nAnnotCols;c<C;c++)
	{
		const char *ach0= ti.GetFirstPointer(nRow, c);
		const char *ach1= ti.GetLastPointer(nRow, c);
		
		Table_SortItem i;
		i.m_nIndex= c;
		i.m_sKey= CString(ach0, ach1-ach0);
		if (bOk)
		{
			bOk= Table_CheckFloatFormat_complete(i.m_sKey)!=NULL;
			if (bOk)
				(void) Table_ScanFloat(i.m_sKey, i.m_fValue);
			else
				i.m_fValue= Stat_GetNaN_float();
		}
		else
			i.m_fValue= 0;

		vItems.SetAt(c-nAnnotCols, i);
	}

	return bOk;
}


bool Table_GetKeyColumn_double(const CTableIndex &ti, 
						int nColumn,
						int nAnnotRows,
						CVector<Table_SortItem_double> &vItems)
{
	// Extract key column.

	// Returns true if all keys are valid floats, false otherwise (too support 
	// choice between alphabetic and numeric sort).
	int R= ti.GetRowCount();
	vItems.SetSize(R-nAnnotRows);
	bool bOk= true;
	for (int r=nAnnotRows;r<R;r++)
	{
		const char *ach0= ti.GetFirstPointer(r, nColumn);
		const char *ach1= ti.GetLastPointer(r, nColumn);
		
		Table_SortItem_double i;
		i.m_nIndex= r;
		i.m_sKey= CString(ach0, ach1-ach0);
		if (bOk)
		{
			bOk= Table_CheckFloatFormat_complete(i.m_sKey)!=NULL;
			if (bOk)
				(void) Table_ScanDouble(i.m_sKey, i.m_Value);
			else
				i.m_Value= Stat_GetNaN_double();
		}
		else
			i.m_Value= 0;

		vItems.SetAt(r-nAnnotRows, i);
	}

	return bOk;
}

bool Table_GetKeyRow_double(const CTableIndex &ti,
					 int nRow,
					 int nAnnotCols,
					 CVector<Table_SortItem_double> &vItems)
{
	// Extract key row

	// Returns true if all keys are valid floats, false otherwise (too support 
	// choice between alphabetic and numeric sort).
	int C= ti.GetMinColCount();
	vItems.SetSize(C-nAnnotCols);
	bool bOk= true;
	for (int c=nAnnotCols;c<C;c++)
	{
		const char *ach0= ti.GetFirstPointer(nRow, c);
		const char *ach1= ti.GetLastPointer(nRow, c);
		
		Table_SortItem_double i;
		i.m_nIndex= c;
		i.m_sKey= CString(ach0, ach1-ach0);
		if (bOk)
		{
			bOk= Table_CheckFloatFormat_complete(i.m_sKey)!=NULL;
			if (bOk)
				(void) Table_ScanDouble(i.m_sKey, i.m_Value);
			else
				i.m_Value= Stat_GetNaN_double();
		}
		else
			i.m_Value= 0;

		vItems.SetAt(c-nAnnotCols, i);
	}

	return bOk;
}

CString g_cmp;
Table_SortItem g_tmp;

void Quicksort_Ascending_String(Table_SortItem *pLeft, Table_SortItem *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem *pi= pLeft-1;
	Table_SortItem *pj= pRight+1;

	g_cmp= pLeft->m_sKey;

	do 
	{
		while ((++pi)->m_sKey<g_cmp);
		while ((--pj)->m_sKey>g_cmp);

		g_tmp= *pi;
		*pi= *pj;
		*pj= g_tmp;
	}
	while (pi<pj);

	g_tmp= *pi;
	*pi= *pj;
	*pj= g_tmp;

	Quicksort_Ascending_String(pLeft, pj);
	Quicksort_Ascending_String(pj+1, pRight);
}

void Quicksort_Descending_String(Table_SortItem *pLeft, Table_SortItem *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem *pi= pLeft-1;
	Table_SortItem *pj= pRight+1;

	g_cmp= pLeft->m_sKey;

	do 
	{
		while ((++pi)->m_sKey>g_cmp);
		while ((--pj)->m_sKey<g_cmp);

		g_tmp= *pi;
		*pi= *pj;
		*pj= g_tmp;
	}
	while (pi<pj);

	g_tmp= *pi;
	*pi= *pj;
	*pj= g_tmp;

	Quicksort_Descending_String(pLeft, pj);
	Quicksort_Descending_String(pj+1, pRight);
}

CString g_cmp_double;
Table_SortItem_double g_tmp_double;

void Quicksort_Ascending_String_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem_double *pi= pLeft-1;
	Table_SortItem_double *pj= pRight+1;

	g_cmp_double= pLeft->m_sKey;

	do 
	{
		while ((++pi)->m_sKey<g_cmp_double);
		while ((--pj)->m_sKey>g_cmp_double);

		g_tmp_double= *pi;
		*pi= *pj;
		*pj= g_tmp_double;
	}
	while (pi<pj);

	g_tmp_double= *pi;
	*pi= *pj;
	*pj= g_tmp_double;

	Quicksort_Ascending_String_double(pLeft, pj);
	Quicksort_Ascending_String_double(pj+1, pRight);
}

void Quicksort_Descending_String_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem_double *pi= pLeft-1;
	Table_SortItem_double *pj= pRight+1;

	g_cmp_double= pLeft->m_sKey;

	do 
	{
		while ((++pi)->m_sKey>g_cmp_double);
		while ((--pj)->m_sKey<g_cmp_double);

		g_tmp_double= *pi;
		*pi= *pj;
		*pj= g_tmp_double;
	}
	while (pi<pj);

	g_tmp_double= *pi;
	*pi= *pj;
	*pj= g_tmp_double;

	Quicksort_Descending_String_double(pLeft, pj);
	Quicksort_Descending_String_double(pj+1, pRight);
}


void Quicksort_Ascending_Numerical(Table_SortItem *pLeft, Table_SortItem *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem *pi= pLeft-1;
	Table_SortItem *pj= pRight+1;

	float v= pLeft->m_fValue;
	Table_SortItem tmp;

	ASSERT(IsN(pLeft->m_fValue));
	ASSERT(IsN(pRight->m_fValue));
	do 
	{
		while ((++pi)->m_fValue<v);
		while ((--pj)->m_fValue>v);

		tmp= *pi;
		*pi= *pj;
		*pj= tmp;
	}
	while (pi<pj);

	tmp= *pi;
	*pi= *pj;
	*pj= tmp;

	Quicksort_Ascending_Numerical(pLeft, pj);
	Quicksort_Ascending_Numerical(pj+1, pRight);
}

void Quicksort_Descending_Numerical(Table_SortItem *pLeft, Table_SortItem *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem *pi= pLeft-1;
	Table_SortItem *pj= pRight+1;

	float v= pLeft->m_fValue;
	Table_SortItem tmp;

	do 
	{
		while ((++pi)->m_fValue>v);
		while ((--pj)->m_fValue<v);

		tmp= *pi;
		*pi= *pj;
		*pj= tmp;
	}
	while (pi<pj);

	tmp= *pi;
	*pi= *pj;
	*pj= tmp;

	Quicksort_Descending_Numerical(pLeft, pj);
	Quicksort_Descending_Numerical(pj+1, pRight);
}

void Quicksort_Ascending_Numerical_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem_double *pi= pLeft-1;
	Table_SortItem_double *pj= pRight+1;

	double v= pLeft->m_Value;
	Table_SortItem_double tmp;

	ASSERT(IsN(pLeft->m_Value));
	ASSERT(IsN(pRight->m_Value));
	do 
	{
		while ((++pi)->m_Value<v);
		while ((--pj)->m_Value>v);

		tmp= *pi;
		*pi= *pj;
		*pj= tmp;
	}
	while (pi<pj);

	tmp= *pi;
	*pi= *pj;
	*pj= tmp;

	Quicksort_Ascending_Numerical_double(pLeft, pj);
	Quicksort_Ascending_Numerical_double(pj+1, pRight);
}

void Quicksort_Descending_Numerical_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight)
{
	if (pRight<=pLeft)
		return;

	Table_SortItem_double *pi= pLeft-1;
	Table_SortItem_double *pj= pRight+1;

	double v= pLeft->m_Value;
	Table_SortItem_double tmp;

	do 
	{
		while ((++pi)->m_Value>v);
		while ((--pj)->m_Value<v);

		tmp= *pi;
		*pi= *pj;
		*pj= tmp;
	}
	while (pi<pj);

	tmp= *pi;
	*pi= *pj;
	*pj= tmp;

	Quicksort_Descending_Numerical_double(pLeft, pj);
	Quicksort_Descending_Numerical_double(pj+1, pRight);
}

void RandomizeItems(Table_SortItem *pSI, int N)
{
	if (N<50)
		return; // Vector is small, don't bother about randomizing

	Table_SortItem tmp;
	srand(1);
	int I= N/3; // 33% permut
	for (int i=0;i<I;i++)
	{
		int j0= rand_int32() % N; // (rand()*N)/RAND_MAX;
		int j1= rand_int32() % N; // (rand()*N)/RAND_MAX;
		tmp= pSI[j0];
		pSI[j0]= pSI[j1];
		pSI[j1]= tmp;
	}
}

void RandomizeItems_double(Table_SortItem_double *pSI, int N)
{
	if (N<50)
		return; // Vector is small, don't bother about randomizing

	Table_SortItem_double tmp;
	srand(1);
	int I= N/3; // 33% permut
	for (int i=0;i<I;i++)
	{
		int j0= rand_int32() % N; // (rand()*N)/RAND_MAX;
		int j1= rand_int32() % N; // (rand()*N)/RAND_MAX;
		tmp= pSI[j0];
		pSI[j0]= pSI[j1];
		pSI[j1]= tmp;
	}
}

void Table_SortItems(Table_SortItem *pSI, int n, bool bAscending, bool bNumerical)
{
	// Randomized quicksort
	RandomizeItems(pSI, n);

	Table_SortItem *pLast= pSI + n-1;
	if (bNumerical)
	{
		if (bAscending)
			Quicksort_Ascending_Numerical(pSI, pLast);
		else
			Quicksort_Descending_Numerical(pSI, pLast);
	}
	else
	{
		if (bAscending)
			Quicksort_Ascending_String(pSI, pLast);
		else
			Quicksort_Descending_String(pSI, pLast);
	}
}


void Table_SortItems_double(Table_SortItem_double *pSI, int n, bool bAscending, bool bNumerical)
{
	// Randomized quicksort
	RandomizeItems_double(pSI, n);

	Table_SortItem_double *pLast= pSI + n-1;
	if (bNumerical)
	{
		if (bAscending)
			Quicksort_Ascending_Numerical_double(pSI, pLast);
		else
			Quicksort_Descending_Numerical_double(pSI, pLast);
	}
	else
	{
		if (bAscending)
			Quicksort_Ascending_String_double(pSI, pLast);
		else
			Quicksort_Descending_String_double(pSI, pLast);
	}
}

void Table_SortItems(CVector<Table_SortItem> &vSI, bool bAscending, bool bNumerical)
{
	// Randomized quicksort (entry-point)
	Table_SortItem *pSI= vSI.GetBuffer();
	Table_SortItems(pSI, vSI.GetSize(), bAscending, bNumerical);
}

Table_SortItem *Table_InitSortItem(CVector<Table_SortItem> &vSI, 
								   CTableIndex &idx, int nAnnotRows, 
								   int nCol, bool bAscending, bool bNumerical)
{
	if (nCol<idx.GetMinColCount())
		return NULL;

	vSI.SetSize(idx.GetRowCount()-nAnnotRows);
	for (int r=nAnnotRows;r<vSI.GetSize();r++)
	{
		Table_SortItem si;
		CString s= Table_GetAt(idx, r, nCol);
		if (bNumerical)
			Table_ScanFloat(s, si.m_fValue);
		else
		{
			si.m_sKey= s;
			si.m_fValue= Stat_GetNaN_float();
		}

		si.m_nIndex= r;
		vSI.SetAt(r-nAnnotRows, si);
	}

	Table_SortItem *pSI= vSI.GetBuffer();
	Table_SortItems(pSI, vSI.GetSize(), bAscending, bNumerical);
	return pSI;
}

/******************************************************************************/
// Format detection

// Automatically detect separating character.
char Table_AutoDetectSeparator(const char *ach, int nSize)
{
	// TODO: Improve. Test a couple of lines:
	// Choose the char that occurs the same number of times on each line?
	int count[256];
	for (int i=0;i<256;i++)
		count[i]= 0;

	const char *ach1= ach + nSize;
	while (*ach && ach<ach1)
		count[int(*ach++)]++;

	int nMax= 0;
	char cMax= 9; // Default is tab

	if (count[9]>nMax) // Tab
	{
		nMax= count[9];
		cMax= 9;
	}
	if (count[int(',')]>nMax) // Comma
	{
		nMax= count[int(',')];
		cMax= ',';
	}
	/*
	if (count[';']>nMax) // Semicolon
	{
		nMax= count[';'];
		cMax= ';';
	}
	*/

	return cMax;
}

char Table_AutoDetectQuotes(const char *ach, char cSeparator)
{
	char cQuote= 0;
	if (*ach=='\"' || *ach=='\'')
		cQuote= *ach;
	else
		return char(0); // Obviously non-quoted.

	// Try parsing the first line.
	while (*ach)
	{
		// Scan quoted data.
		if (*ach!=cQuote)
			return char(0);
		ach++;

		while (*ach && *ach!=cQuote && *ach!=10 && *ach!=13)
			ach++;
		if (*ach!=cQuote)
			return char(0);
		ach++;

		// Past quotes => either eol, eof or new separator.
		if (*ach==0 || *ach==10 || *ach==13)
			return cQuote;

		// Check for separator character.
		if (*ach!=cSeparator)
			return char(0);
		ach++;
	}

	return char(0);
}

char Table_SeparatorFromFileName(const char *ach)
{
	if (strcmp_nocase(ach, ".csv")==0)
		return ',';
	if (strcmp_nocase(ach, ".txt")==0)
		return '\t';
	return '\t'; // Default is tab.
}

bool Table_EnumerateClasses(const CTableIndex &ti, int nRow, int nAnnotCols, 
							CVector<CString> &vClassNames, 
							CVector<int> &vClassCount, 
							CVector<int> &vColumnClass)
{
	// Enumerates the classes on a given row and counts the members.
	//
	// NOTE: vClassNames and vClassCount are allowed to be non-empty from start.

	vColumnClass.SetSize(0);
	while (vClassCount.GetSize()<vClassNames.GetSize())
		vClassCount.Add(0);

	for (int i=nAnnotCols;i<ti.GetColCount(nRow);i++)
	{
		const char *ach0= ti.GetFirstPointer(nRow, i);
		const char *ach1= ti.GetLastPointer(nRow, i);
		CString sName= CString(ach0, ach1-ach0);
		/*
		if (sName.IsEmpty())
			return false;
		*/

		int j;
		for (j=vClassNames.GetSize();--j>=0;)
			if (vClassNames[j]==sName)
				break;

		if (j<0)
		{
			j= vClassNames.GetSize();
			vClassNames.Add(CString(ach0, ach1-ach0));
			vClassCount.Add(0);
		}

		vClassCount.SetAt(j, vClassCount[j]+1);
		vColumnClass.Add(j);
	}

	return true;
}

CString Table_GetAt(const CTableIndex &i, const int r, const int c)
{
	return CString(i.GetFirstPointer(r,c), i.GetLastPointer(r,c)-i.GetFirstPointer(r,c));
}

#ifdef _WIN32
__int64 Table_GetLength(const CTableIndex &i, const int r, const int c)
{
	return i.GetLastPointer(r,c)-i.GetFirstPointer(r,c);
}
#else
int64_t Table_GetLength(const CTableIndex &i, const int r, const int c)
{
	return i.GetLastPointer(r,c)-i.GetFirstPointer(r,c);
}
#endif


bool Table_GetFloatAt(const CTableIndex &i, const int r, const int c, float &value)
{
	// Slow. To convert full matrices, use Table_ScanFloatMatrix instead.
	CString s= Table_GetAt(i, r, c);
	if (Table_CheckFloatFormat(s)!=NULL && Table_ScanFloat(s, value))
		return true;
	return false;
}

bool Table_GetIntAt(const CTableIndex &i, const int r, const int c, int &value)
{
	// Slow. Do not use on full matrices.
	CString s= Table_GetAt(i, r, c);
	if (Table_CheckIntFormat(s)!=NULL && sscanf(s, "%d", &value)>0)
		return true;
	return false;
}

const char *Table_ScanColumnName(const char *ach, CTableIndex &ti, int &nCol)
{
	const int r= 0;
	for (int c=0;c<ti.GetColCount(r);c++)
	{
		size_t n= ti.GetLastPointer(r,c)-ti.GetFirstPointer(r,c);
		if (strncmp(ach, ti.GetFirstPointer(r,c), n)==0)
		{
			nCol= c;
			return ach+n;
		}
	}
	return NULL;
}

const char *Table_ScanRowName(const char *ach, CTableIndex &ti, int &nRow)
{
	const int c= 0;
	for (int r=0;r<ti.GetRowCount();r++)
	{
		size_t n= ti.GetLastPointer(r,c)-ti.GetFirstPointer(r,c);
		if (strncmp(ach, ti.GetFirstPointer(r,c), n)==0)
		{
			nRow= r;
			return ach+n;
		}
	}
	return NULL;
}

bool IsDollarName(const CString &s)
{
	if (s.GetLength()>0 && s[0]=='$')
		return true;
	return false;
}

bool IsQuotedName(const CString &s)
{
	if (s.GetLength()>2)
	{
		if ((s[0]=='\"' || s[0]=='\'') && s[s.GetLength()-1]==s[0])
			return true;
	}
	return false;
}

int Table_FindAnnotRow(const CTableIndex &ti, const CString &sName, int nAnnotRows)
{
	return Table_FindAnnotRow(ti, 0, sName, nAnnotRows);
}

int Table_FindAnnotRow(const CTableIndex &ti, int nCol, const CString &sName, int nAnnotRows)
{
	if (ti.IsEmpty())
		return -1;

	CString s;
	if (IsQuotedName(sName))
		s= sName.Mid(1, sName.GetLength()-2);
	else if (IsDollarName(sName))
		s= sName.Right(sName.GetLength()-1);
	else
		s= sName;

	for (int i=0;i<nAnnotRows;i++)
		if (Table_GetAt(ti,i,nCol)==s)
			return i;
	return -1;
}

int Table_FindAnnotCol(const CTableIndex &ti, const CString &sName, int nAnnotCols)
{
	return Table_FindAnnotCol(ti, 0, sName, nAnnotCols);
}

int Table_FindAnnotCol(const CTableIndex &ti, int nRow, const CString &sName, int nAnnotCols)
{
	if (ti.IsEmpty())
		return -1;

	CString s;
	if (IsQuotedName(sName))
		s= sName.Mid(2, sName.GetLength()-2);
	else if (IsDollarName(sName))
		s= sName.Right(sName.GetLength()-1);
	else
		s= sName;

	for (int i=0;i<nAnnotCols;i++)
		if (Table_GetAt(ti,nRow,i)==s)
			return i;
	return -1;
}

int Table_FindRow(const CTableIndex &ti, const CString &sName)
{ 
	return Table_FindRow(ti, 0, sName); 
}

int Table_FindCol(const CTableIndex &ti, const CString &sName)
{ 
	return Table_FindCol(ti, 0, sName); 
}

int Table_FindRow(const CTableIndex &ti, int nCol, const CString &sName)
{
	return Table_FindAnnotRow(ti, nCol, sName, ti.GetRowCount());
}

int Table_FindCol(const CTableIndex &ti, int nRow, const CString &sName)
{
	return Table_FindAnnotCol(ti, nRow, sName, ti.GetColCount(0));
}

int Table_FindRowEx(const CTableIndex &ti, const CVector<CString> &vName)
{
	for (int i=0;i<vName.GetSize();i++)
	{
		CString sName= vName[i];
		int n= Table_FindAnnotRow(ti, sName, ti.GetRowCount());
		if (n>=0)
			return n;
	}
	return -1;
}

int Table_FindColEx(const CTableIndex &ti, const CVector<CString> &vName)
{
	for (int i=0;i<vName.GetSize();i++)
	{
		CString sName= vName[i];
		int n= Table_FindAnnotCol(ti, sName, ti.GetColCount(0));
		if (n>=0)
			return n;
	}
	return -1;
}

int Table_ResolveAnnotCol(const CTableIndex &ti, CString &sName, int nAnnotCols)
{
	// Returns: Index of column sName or -1. Translates sName if necessary.
	if (!IsQuotedName(sName) && !IsDollarName(sName))
	{
		int c;
		const char *ach0= sName;
		const char *ach1= Scan_UnsignedInt(sName, &c);
		if (ach1==ach0+sName.GetLength())
		{
			if (c<0 || c>=nAnnotCols)
				return -2;
			if (c<ti.GetColCount(0))
				sName= Table_GetAt(ti, 0, c);
			else
				sName= "";
			return c;
		}
		if (ti.IsEmpty())
			return -1;
	}
	return Table_FindAnnotCol(ti, sName, nAnnotCols);
}

int Table_ResolveAnnotRow(const CTableIndex &ti, CString &sName, int nAnnotRows)
{
	// Returns: Index of row sName or -1. Translates sName if necessary.
	if (!IsQuotedName(sName) && !IsDollarName(sName))
	{
		int r;
		const char *ach0= sName;
		const char *ach1= Scan_UnsignedInt(sName, &r);
		if (ach1==ach0+sName.GetLength())
		{
			if (r<0 || r>=nAnnotRows)
				return -2;
			if (r<ti.GetRowCount())
				sName= Table_GetAt(ti, r, 0);
			else
				sName= "";
			return r;
		}
		if (ti.IsEmpty())
			return -1;
	}
	return Table_FindAnnotRow(ti, sName, nAnnotRows);
}

int Table_ResolveCol(const CTableIndex &ti, CString &sName)
{
	return Table_ResolveAnnotCol(ti, sName, ti.GetColCount(0));
}

int Table_ResolveRow(const CTableIndex &ti, CString &sName)
{
	return Table_ResolveAnnotRow(ti, sName, ti.GetRowCount());
}

int Table_ResolveColEx(const CTableIndex &ti, const CVector<CString> &vName)
{
	for (int i=0;i<vName.GetSize();i++)
	{
		CString sName= vName[i];
		int n= Table_ResolveCol(ti, sName);
		if (n>=0)
			return n;
	}
	return -1;
}

int Table_ResolveColEx(const CTableIndex &ti, const CString &sList)
{
	CVector<CString> vName;
	SplitAtChar(sList, '|', vName);
	return Table_ResolveColEx(ti, vName);
}

int Table_ResolveRowEx(const CTableIndex &ti, const CVector<CString> &vName)
{
	for (int i=0;i<vName.GetSize();i++)
	{
		CString sName= vName[i];
		int n= Table_ResolveRow(ti, sName);
		if (n>=0)
			return n;
	}
	return -1;
}

int Table_ResolveRowEx(const CTableIndex &ti, const CString &sList)
{
	CVector<CString> vName;
	SplitAtChar(sList, '|', vName);
	return Table_ResolveRowEx(ti, vName);
}

bool Table_CenterByClass(const CTableIndex &ti, const int nAnnotCols, CMatrix<float> &mData, const int nClassRow)
{
	CVector<CString> vClassNames;
	CVector<int> vClassCount;
	CVector<int> vColumnClass;
	if (!Table_EnumerateClasses(ti, nClassRow, nAnnotCols, vClassNames, vClassCount, vColumnClass) || vClassNames.GetSize()==0)
		return false;
	if (vColumnClass.GetSize()!=mData.GetWidth())
		return false;

	CVector<double> vs;
	vs.SetSize(vClassNames.GetSize());
	double *ps= vs.GetBuffer();

	CVector<int> vn;
	vn.SetSize(vClassNames.GetSize());
	int *pn= vn.GetBuffer();

	for (int r=0;r<mData.GetHeight();r++)
	{
		float *pr= mData.GetPointer(r, 0);

		// compute means
		for (int c=0;c<vClassNames.GetSize();c++)
		{
			ps[c]= 0;
			pn[c]= 0;
		}
		for (int i=0;i<vColumnClass.GetSize();i++)
		{
			int c= vColumnClass[i];
			ps[c] += pr[i];
			pn[c] ++;
		}
		for (int c=0;c<vClassNames.GetSize();c++)
			ps[c] /= pn[c];

		// center
		for (int i=0;i<vColumnClass.GetSize();i++)
			pr[i] -= float(vs[vColumnClass[i]]);
	}

	return true;
}

int Table_FindInIndex(const char *ach, const CVector<Table_SortItem> &vSI /* sorted by m_sKey in ascending order */)
{
	// Find string in vSI::m_sKey using binary search
	if (!vSI.GetSize())
		return -1;

	int r= -1; 
	if (ach && *ach)
	{
		int i0= 0;
		int i1= vSI.GetSize()-1;
		int imid, sgn;
		do
		{
			imid= (i0+i1) >> 1;
			sgn= strcmp(ach, vSI[imid].m_sKey);
			if (sgn==0)
				break;
			if (sgn>0)
				i0= imid+1;
			else 
				i1= imid;
		}
		while (i0<i1);
		if (i0==i1)
		{
			sgn= strcmp(ach, vSI[i0].m_sKey);
			imid= i0;
		}

		if (sgn==0)
			r= vSI[imid].m_nIndex;
		else
			r= -1;
	}
	return r;
}

