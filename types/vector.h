/***************************************************************************
 *
 *   vector.h -- Reference counted vector class
 *
 *   Erik Persson and Bj�rn Nilsson, 2000-2008
 */

#ifndef TYPES_VECTOR_H
#define TYPES_VECTOR_H

#include <new>
#include <string.h> // For memcpy
#include "types/assert.h"

/***************************************************************************/

namespace types // Hide internal functions
{
template<class TYPE>
inline void ConstructArray(TYPE *pElements, int nSize)
{
	// Initialize memory to zero before calling constructor
	memset((void *) pElements, 0x00, nSize*sizeof(TYPE));

	// Call constructor for every element
	while (nSize--)
		::new((void *) (pElements++)) TYPE;
}

template<class TYPE>
inline void DestructArray(TYPE *pElements, int nSize)
{
	// Call destructor for every element
	while (nSize--)
		(pElements++)->~TYPE();
}

template<class TYPE>
inline void CopyArray(TYPE *pDest, const TYPE *pSrc, int nSize)
{
	// Call assignment operator for every element
	while (nSize--)
		*pDest++ = *pSrc++;
}

// Override for simple types
template<> inline void ConstructArray<int>(int *pElements, int nSize) {}
template<> inline void ConstructArray<unsigned int>(unsigned int *pElements, int nSize) {}
template<> inline void ConstructArray<char>(char *pElements, int nSize) {}
template<> inline void ConstructArray<unsigned char>(unsigned char *pElements, int nSize) {}
template<> inline void DestructArray<int>(int *pElements, int nSize) {}
template<> inline void DestructArray<unsigned int>(unsigned int *pElements, int nSize) {}
template<> inline void DestructArray<char>(char *pElements, int nSize) {}
template<> inline void DestructArray<unsigned char>(unsigned char *pElements, int nSize) {}
template<> inline void CopyArray<int>(int *pDest, const int *pSrc, int nSize) { memcpy(pDest,pSrc,nSize*sizeof(int));}
template<> inline void CopyArray<unsigned int>(unsigned int *pDest, const unsigned int *pSrc, int nSize) {memcpy(pDest,pSrc,nSize*sizeof(unsigned int));}
template<> inline void CopyArray<char>(char *pDest, const char *pSrc, int nSize) {memcpy(pDest,pSrc,nSize*sizeof(char));}
template<> inline void CopyArray<unsigned char>(unsigned char *pDest, const unsigned char *pSrc, int nSize) {memcpy(pDest,pSrc,nSize*sizeof(unsigned char));}

}; // end namespace types

/***************************************************************************/

struct CVectorData
{
	int nRefs;       // Reference count
	int nSize;       // Length of data (including terminator)
	int nMaxSize;    // Length of allocation

  // Elements are placed after this structure
};

// Global Empty Vector Data
extern void *g_pNilVectorElements;

/***************************************************************************/

template <class T>
class CVector
{
  // Members
protected:
	mutable T *m_pElements;                             // Pointer to shared vector data

public:
// Construction & Destruction
	CVector();                                          // Constructs empty CVector
	CVector(const CVector<T>& src);                     // Copy constructor (ref-counted copy)
	CVector(const T *pArray, int nSize);                // Construct from array
	CVector(int nSize);									// Construct array of a given size
	virtual ~CVector();

// Properties
public:
	bool IsEmpty() const;								// Returns true for an empty vector.
	int GetSize() const;                                // Get number of elements currently in vector
	void SetSize(int nNewSize);                         // Set number of elements currently in vector
	const T& GetAt(int nIndex) const;
	void SetAt(int nIndex, const T& newElement);
	operator const T *() const;                         // Return pointer to const array

// General Methods	
public:
	const T& operator[](int nIndex) const;
	const CVector<T>& operator=(const CVector<T>& src); // Ref-counted copy from another CVector
	void Add(const T& newElement);                      // Append one element after the vector
	void Append(const CVector<T>& src);					// Append another vector
	T *GetBuffer(int nMinSize);							// Get pointer to modifiable buffer
	T *GetBuffer();										// Get pointer to modifiable buffer with the same size as the vector
	void Delete(int nPosition, int nCount=1);			// Delete one or more elements inside the vector
	void Insert(int nPosition, const T& newElement);	// Insert one element inside the vector
	void InsertVector(int nPosition, const CVector<T> &src); // Insert a vector inside the vector
	void Fill(T data); // Fill vector with one element.
	int Find(T data) const; // Query the (lowest) index of an element in the vector
	// Implementation helpers
protected:
  CVectorData *GetData() const;
	void OwnData();                                     // Make sure ref count is one

  // To be thread-safe: assure that these two are atomic operations
  void ClaimData();
  void ReleaseData();
};

/*********** Function definitions ***********/

template <class T>
T *CreateBuffer(int nSize, int nMaxSize)
{
  // Create CVectorData and return pointer to elements
  ASSERT(nSize > 0);
  ASSERT(nMaxSize >= nSize);

  CVectorData *pData= (CVectorData *) new char[sizeof(CVectorData) + nMaxSize * sizeof(T)];
  pData->nRefs = 1;
	pData->nSize = nSize;
	pData->nMaxSize= nMaxSize;
  
  T *pElements= (T *) (pData+1);
	types::ConstructArray<T>(pElements, nSize); // Construct only nSize elements

  return pElements;
}

template <class T>
inline CVectorData *CVector<T>::GetData() const   { return ((CVectorData *) m_pElements)-1; }

template <class T>
inline CVector<T>::CVector()                      { m_pElements= (T *) g_pNilVectorElements; }

template <class T>
inline CVector<T>::CVector(const CVector<T>& src) { m_pElements= src.m_pElements; ClaimData(); }

template <class T>
inline CVector<T>::CVector(const T *pArray, int nSize)
{ 
	if (nSize>0)
	{
    T *pNewElements= CreateBuffer<T>(nSize,nSize);
		types::CopyArray(pNewElements,pArray,nSize);
    m_pElements= pNewElements;
	}
	else
	{
		ASSERT(nSize==0);
		m_pElements= (T *) g_pNilVectorElements;
	}
}

template <class T>
inline CVector<T>::CVector(int nSize)
{
	if (nSize>0)
	{
	    m_pElements= CreateBuffer<T>(nSize,nSize);	
	}
	else
	{
		ASSERT(nSize==0);
		m_pElements= (T *) g_pNilVectorElements;
	}
}

template <class T>
inline CVector<T>::~CVector()                     { ReleaseData(); }

template <class T>
inline const T& CVector<T>::GetAt(int nIndex) const
{	ASSERT(nIndex >= 0 &&	nIndex < GetData()->nSize);
	return m_pElements[nIndex];
}

template <class T>
inline void CVector<T>::SetAt(int nIndex, const T& newElement)
{	
	ASSERT(nIndex >= 0 && nIndex < GetData()->nSize);
  OwnData();
	m_pElements[nIndex]= newElement;
}

template <class T>
inline const T& CVector<T>::operator[](int nIndex) const
{	// Same as GetAt
	ASSERT(nIndex >= 0 && nIndex < GetData()->nSize);
	return m_pElements[nIndex];
}

template <class T>
inline const CVector<T>& CVector<T>::operator=(const CVector<T>& src)
{
  ReleaseData();
  m_pElements= src.m_pElements;
  ClaimData();
  return *this;
}

template <class T>
inline CVector<T>::operator const T *() const
{
  return m_pElements;
}

template <class T>
inline int CVector<T>::GetSize() const { return GetData()->nSize; }

template <class T>
inline bool CVector<T>::IsEmpty() const { return GetSize()==0; }

template <class T>
void CVector<T>::Add(const T& newElement)
{	
  int nIndex= GetSize();
  SetSize(nIndex+1);
	m_pElements[nIndex]= newElement; // No OwnData needed after changing size
}

template <class T>
void CVector<T>::Append(const CVector<T>& src)
{
	// Note that src and this may be the same instance
	if (int nAppendSize= src.GetSize())
	{
		int nOldSize = GetSize();
		SetSize(nOldSize + nAppendSize);
		// No OwnData needed after changing size
		types::CopyArray<T>(m_pElements + nOldSize, src.m_pElements, nAppendSize);
	}
}

template <class T>
T *CVector<T>::GetBuffer(int nMinSize)
{	
	if (nMinSize)
	{
		if (GetSize() < nMinSize)
			SetSize(nMinSize); // No OwnData needed after changing size
		else 
			OwnData();
	}
	return (T *) m_pElements;
}

template <class T>
T *CVector<T>::GetBuffer()
{
	return GetBuffer(GetSize());
}

template <class T>
inline void CVector<T>::ClaimData()
{ 
  ++(GetData()->nRefs);
}

template <class T>
inline void CVector<T>::ReleaseData()
{ 
  if (!(--(GetData()->nRefs)))
  {
		types::DestructArray<T>(m_pElements, GetSize());
    delete[] GetData();
  }
}

template <class T>
void CVector<T>::OwnData()
{
  if (GetData()->nRefs > 1)
  {
    T *pNewElements= CreateBuffer<T>(GetData()->nSize, GetData()->nMaxSize);
		types::CopyArray(pNewElements,m_pElements,GetData()->nSize);
    ReleaseData();
    m_pElements= pNewElements;
  }
}

template <class T>
void CVector<T>::SetSize(int nNewSize)
{
	ASSERT(nNewSize >= 0);
	CVectorData *pData= GetData();
  
	int nSize= pData->nSize;
  if (!nNewSize)
	{
		// Release vector data
    ReleaseData();
    m_pElements= (T *) g_pNilVectorElements; // No ClaimData() needed for this one
	}
	else if (nNewSize == nSize || pData->nRefs <= 1 && nNewSize <= pData->nMaxSize)
	{
    // No need to reallocate buffer. Resize
		if (nNewSize > nSize)
		{
			// initialize the new elements
			types::ConstructArray<T>(&m_pElements[nSize], nNewSize-nSize);
		}
		else if (nSize > nNewSize)
		{
			// destroy the old elements
			types::DestructArray<T>(&m_pElements[nNewSize], nSize-nNewSize);
		}
		pData->nSize= nNewSize;
  }
  else
	{
		// Reallocate buffer. Heuristically determine growth
		int nGrowBy = pData->nSize / 4;
		nGrowBy = (nGrowBy < 4) ? 4 : nGrowBy;
		int nNewMax;
		if (nNewSize < pData->nMaxSize + nGrowBy)
			nNewMax = pData->nMaxSize + nGrowBy;  // granularity
		else
			nNewMax = nNewSize;  // no slush

    T *pNewElements= CreateBuffer<T>(nNewSize, nNewMax);
    int nCommonSize= nNewSize;
    if (nCommonSize>nSize) nCommonSize= nSize;
		types::CopyArray(pNewElements,m_pElements,nCommonSize);
    ReleaseData();
    m_pElements= pNewElements;
	}
}

template <class T>
void CVector<T>::Delete(int nPosition, int nCount)
{
	// Delete nCount elements at nPosition.
	if (nPosition>=0 && nPosition<GetSize())
	{
		if (nCount<=0)
			return;
		OwnData();

		int i= nPosition;
		int j= i+nCount;
		while (j < GetSize())
			m_pElements[i++]= m_pElements[j++];

		SetSize(i);
	}
	else
		ASSERT(false);
}

template <class T>
void CVector<T>::Insert(int nPosition, const T& newElement)
{
	// Inserts the element T at nPosition. The old element and
	// the trailing elements will be shifted upwards. 
	// If nPosition==GetSize(), the call will be equivalent to Add().
	if (nPosition<0 || nPosition>GetSize())
	{
		ASSERT(false);
		return;
	}

	int nSize= GetSize();
	SetSize(nSize+1); // No OwnData needed after changing size

	for (int i=nSize;i>nPosition;i--)
		m_pElements[i]= m_pElements[i-1];

	m_pElements[nPosition]= newElement; 
}

template <class T>
void CVector<T>::InsertVector(int nPosition, const CVector<T> &src)
{
	// Inserts a vector at nPosition. The old elements at 
	// nPosition and up will be shifted upwards.
	// If nPosition==GetSize(), the call will be equivalent to Append().
	if (nPosition<0 || nPosition>GetSize())
	{
		ASSERT(false);
		return;
	}

	if (int nAppendSize= src.GetSize())
	{
		int nSize = GetSize() + nAppendSize;
		SetSize(nSize); // No OwnData needed after changing size

		int i= nSize;
		int j= i-nAppendSize;
		while (j>nPosition)
			m_pElements[--i]= m_pElements[--j];

		types::CopyArray<T>(m_pElements + nPosition, src.m_pElements, nAppendSize);
	}
}

template <class T>
void CVector<T>::Fill(T data)
{
	for (int i=GetSize();--i>=0;)
		m_pElements[i]= data;
}

template <class T>
int CVector<T>::Find(T data) const
{
	for (int i=GetSize();--i>=0;)
		if (m_pElements[i]==data)
			return i;
	return -1;
}

#endif // TYPES_VECTOR_H
