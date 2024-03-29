/*******************************************************************************
 *
 *   tablefun.h -- Common operations on table indices
 *
 *   Bj�rn Nilsson, 2004-2009
 */

#ifndef TABLEFUN_H
#define TABLEFUN_H

#include "tableindex.h"
#include "matrix.h"
#include "system/filesystem.h"
// #include <stdint.h> 

/******************************************************************************/

// Table Flags (for matrix diagnostics)
#define TF_Missing (1)
#define TF_Error (2)
#define TF_Custom (4) 

struct Table_SortItem
{
	CString m_sKey; // For string-based sorts
	float m_fValue; // For numerical sorts
	int m_nIndex;
};

struct Table_SortItem_double
{
	CString m_sKey; // For string-based sorts
	double m_Value; // For numerical sorts
	int m_nIndex;
};


/******************************************************************************/

// Convenience functions for standard loading and parsing
CLoadBuf *Table_LoadTabDataset(CString sName, CTableIndex &ti, CString &sErr);
CLoadBuf *Table_LoadTabDatasetHeader(CString sFilename, CTableIndex &ti, CString &sErr);
bool Table_ScanTabDataset(const CTableIndex &idx, int &nAnnotRows, int &nAnnotCols, CMatrix<float> &mData);

// Element access
CString Table_GetAt(const CTableIndex &i, const int r, const int c);
#ifdef _WIN32
__int64 Table_GetLength(const CTableIndex &i, const int r, const int c);
#else
int64_t Table_GetLength(const CTableIndex &i, const int r, const int c);
#endif
bool Table_GetFloatAt(const CTableIndex &i, const int r, const int c, float &value);
bool Table_GetIntAt(const CTableIndex &i, const int r, const int c, int &value);

//
char Table_AutoDetectSeparator(const char *ach, int nSize);
char Table_AutoDetectQuotes(const char *ach, char cSeparator);
char Table_SeparatorFromFileName(const char *ach);
bool Table_DetectAnnotation(const CTableIndex &ti, 
								int &nAnnotRows, 
								int &nAnnotCols, 
								CMatrix<short> &mFlags);
bool Table_ScanFloat(const char *ach, float &x);
bool Table_ScanDouble(const char *ach, double &x);
const char *Table_CheckFloatFormat(const char *ach);
const char *Table_CheckFloatFormat_complete(const char *ach);
const char *Table_CheckIntFormat(const char *ach);
const char *Table_CheckIntFormat_complete(const char *ach);
const char *Table_CheckUnsignedIntFormat(const char *ach);
const char *Table_CheckUnsignedIntFormat_complete(const char *ach);

//
bool Table_ScanFloatMatrix(const CTableIndex &ti,
						  int row0, int col0,
						  int row1, int col1,
						  CMatrix<float> &pValues, 
						  CMatrix<short> *pFlags);
void Table_FindByFlags(const CMatrix<short> &mFlags, short mask, int &row, int &col);

// Sorting
bool Table_GetKeyColumn(const CTableIndex &ti, 
						int nColumn,
						int nAnnotRows,
						CVector<Table_SortItem> &vItems);
bool Table_GetKeyRow(const CTableIndex &ti,
					 int nRow,
					 int nAnnotCols,
					 CVector<Table_SortItem> &vItems);
bool Table_GetKeyColumn_double(const CTableIndex &ti, 
						int nColumn,
						int nAnnotRows,
						CVector<Table_SortItem_double> &vItems);
bool Table_GetKeyRow_double(const CTableIndex &ti,
					 int nRow,
					 int nAnnotCols,
					 CVector<Table_SortItem_double> &vItems);
void Table_SortItems(CVector<Table_SortItem> &vSI, bool bAscending, bool bNumerical);
void Table_SortItems(Table_SortItem *pSI, int n, bool bAscending, bool bNumerical);
void Table_SortItems_double(Table_SortItem_double *pSI, int n, bool bAscending, bool bNumerical);

void Quicksort_Ascending_String(Table_SortItem *pLeft, Table_SortItem *pRight);
void Quicksort_Descending_String(Table_SortItem *pLeft, Table_SortItem *pRight);
//void Quicksort_Ascending_String_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight);
//void Quicksort_Descending_String_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight);
void Quicksort_Ascending_Numerical(Table_SortItem *pLeft, Table_SortItem *pRight);
void Quicksort_Descending_Numerical(Table_SortItem *pLeft, Table_SortItem *pRight);
//void Quicksort_Ascending_Numerical_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight);
//void Quicksort_Descending_Numerical_double(Table_SortItem_double *pLeft, Table_SortItem_double *pRight);

// Finding columns and rows from name
int Table_FindRow(const CTableIndex &ti, int nCol, const CString &sName);
int Table_FindCol(const CTableIndex &ti, int nRow, const CString &sName);
int Table_FindRow(const CTableIndex &ti, const CString &sName);
int Table_FindCol(const CTableIndex &ti, const CString &sName);
int Table_FindRowEx(const CTableIndex &ti, const CVector<CString> &vName);
int Table_FindColEx(const CTableIndex &ti, const CVector<CString> &vName);
int Table_FindAnnotRow(const CTableIndex &ti, const CString &sName, int nAnnotRows);
int Table_FindAnnotCol(const CTableIndex &ti, const CString &sName, int nAnnotCols);
int Table_FindAnnotRow(const CTableIndex &ti, int nCol, const CString &sName, int nAnnotRows);
int Table_FindAnnotCol(const CTableIndex &ti, int nRow, const CString &sName, int nAnnotCols);
int Table_ResolveAnnotRow(const CTableIndex &ti, CString &sName, int nAnnotRows);
int Table_ResolveAnnotCol(const CTableIndex &ti, CString &sName, int nAnnotCols);
int Table_ResolveRow(const CTableIndex &ti, CString &sName);
int Table_ResolveCol(const CTableIndex &ti, CString &sName);
int Table_ResolveRowEx(const CTableIndex &ti, const CVector<CString> &vName);
int Table_ResolveColEx(const CTableIndex &ti, const CVector<CString> &vName);
int Table_ResolveRowEx(const CTableIndex &ti, const CString &sList);
int Table_ResolveColEx(const CTableIndex &ti, const CString &sList);
int Table_FindInIndex(const char *ach, const CVector<Table_SortItem> &vSI /* sorted by m_sKey in ascending order */);

const char *Table_ScanColumnName(const char *ach, CTableIndex &ti, int &nCol);
const char *Table_ScanRowName(const char *ach, CTableIndex &ti, int &nRow);

// Misc
bool Table_EnumerateClasses(const CTableIndex &ti, int nRow, int nAnnotCols, 
							CVector<CString> &vClassNames, 
							CVector<int> &vClassCount, 
							CVector<int> &vColumnClass);
bool Table_Resort(const CTableIndex &ti, CVector<Table_SortItem> vSI);
bool Table_CenterByClass(const CTableIndex &ti, const int nAnnotCols, CMatrix<float> &mData, const int nClassRow);


#endif
