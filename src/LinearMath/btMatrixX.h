/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2013 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
///original version written by Erwin Coumans, October 2013

#ifndef BT_MATRIX_X_H
#define BT_MATRIX_X_H

#include "LinearMath/btQuickprof.h"
#include "LinearMath/btAlignedObjectArray.h"

//#define BT_DEBUG_OSTREAM
#ifdef BT_DEBUG_OSTREAM
#include <iostream>
#include <iomanip>      // std::setw
#endif //BT_DEBUG_OSTREAM

class btIntSortPredicate
{
	public:
		bool operator() ( const int& a, const int& b ) const
		{
			 return a < b;
		}
};


template <typename T>
struct btVectorX
{
	btAlignedObjectArray<T>	m_storage;
	
	btVectorX()
	{
	}
	btVectorX(int numRows)
	{
		m_storage.resize(numRows);
	}
	
	void resize(int rows)
	{
		m_storage.resize(rows);
	}
	int cols() const
	{
		return 1;
	}
	int rows() const
	{
		return m_storage.size();
	}
	int size() const
	{
		return rows();
	}
	
	T nrm2() const
	{
		T norm = T(0);
		
		int nn = rows();
		
		{
			if (nn == 1)
			{
				norm = btFabs((*this)[0]);
			}
			else
			{
				T scale = 0.0;
				T ssq = 1.0;
				
				/* The following loop is equivalent to this call to the LAPACK
				 auxiliary routine:   CALL SLASSQ( N, X, INCX, SCALE, SSQ ) */
				
				for (int ix=0;ix<nn;ix++)
				{
					if ((*this)[ix] != 0.0)
					{
						T absxi = btFabs((*this)[ix]);
						if (scale < absxi)
						{
							T temp;
							temp = scale / absxi;
							ssq = ssq * (temp * temp) + 1.0;
							scale = absxi;
						}
						else
						{
							T temp;
							temp = absxi / scale;
							ssq += temp * temp;
						}
					}
				}
				norm = scale * sqrt(ssq);
			}
		}
		return norm;
		
	}
	void	setZero()
	{
		//	for (int i=0;i<m_storage.size();i++)
		//		m_storage[i]=0;
		//memset(&m_storage[0],0,sizeof(T)*m_storage.size());
		btSetZero(&m_storage[0],m_storage.size());
	}
	const T& operator[] (int index) const
	{
		return m_storage[index];
	}
	
	T& operator[] (int index)
	{
		return m_storage[index];
	}
	
	T* getBufferPointerWritable()
	{
		return m_storage.size() ? &m_storage[0] : 0;
	}
	
	const T* getBufferPointer() const
	{
		return m_storage.size() ? &m_storage[0] : 0;
	}
	
};
/*
 template <typename T>
 void setElem(btMatrixX<T>& mat, int row, int col, T val)
 {
 mat.setElem(row,col,val);
 }
 */


template <typename T> 
struct btMatrixX
{
	int m_rows;
	int m_cols;
	int m_operations;
	int m_resizeOperations;
	int m_setElemOperations;

	btAlignedObjectArray<T>	m_storage;
	btAlignedObjectArray< btAlignedObjectArray<int> > m_rowNonZeroElements1;
	btAlignedObjectArray< btAlignedObjectArray<int> > m_colNonZeroElements;

	T* getBufferPointerWritable() 
	{
		return m_storage.size() ? &m_storage[0] : 0;
	}

	const T* getBufferPointer() const
	{
		return m_storage.size() ? &m_storage[0] : 0;
	}
	btMatrixX()
		:m_rows(0),
		m_cols(0),
		m_operations(0),
		m_resizeOperations(0),
		m_setElemOperations(0)
	{
	}
	btMatrixX(int rows,int cols)
		:m_rows(rows),
		m_cols(cols),
		m_operations(0),
		m_resizeOperations(0),
		m_setElemOperations(0)
	{
		resize(rows,cols);
	}
	void resize(int rows, int cols)
	{
		m_resizeOperations++;
		m_rows = rows;
		m_cols = cols;
		{
			BT_PROFILE("m_storage.resize");
			m_storage.resize(rows*cols);
		}
		clearSparseInfo();
	}
	int cols() const
	{
		return m_cols;
	}
	int rows() const
	{
		return m_rows;
	}
	///we don't want this read/write operator(), because we cannot keep track of non-zero elements, use setElem instead
	/*T& operator() (int row,int col)
	{
		return m_storage[col*m_rows+row];
	}
	*/

	void addElem(int row,int col, T val)
	{
		if (val)
		{
			if (m_storage[col+row*m_cols]==0.f)
			{
				setElem(row,col,val);
			} else
			{
				m_storage[row*m_cols+col] += val;
			}
		}
	}
	
	
	void setElem(int row,int col, T val)
	{
		m_setElemOperations++;
		if (val)
		{
			if (m_storage[col+row*m_cols]==0.f)
			{
				m_rowNonZeroElements1[row].push_back(col);
				m_colNonZeroElements[col].push_back(row);
			}
		}
		m_storage[row*m_cols+col] = val;
	}
	
	void mulElem(int row,int col, T val)
	{
		m_setElemOperations++;
		//mul doesn't change sparsity info

		m_storage[row*m_cols+col] *= val;
	}
	
	
	
	
	void copyLowerToUpperTriangle()
	{
		int count=0;
		for (int row=0;row<m_rowNonZeroElements1.size();row++)
		{
			for (int j=0;j<m_rowNonZeroElements1[row].size();j++)
			{
				int col = m_rowNonZeroElements1[row][j];
				setElem(col,row, (*this)(row,col));
				count++;
				
			}
		}
		//printf("copyLowerToUpperTriangle copied %d elements out of %dx%d=%d\n", count,rows(),cols(),cols()*rows());
	}
	
	const T& operator() (int row,int col) const
	{
		return m_storage[col+row*m_cols];
	}

	void clearSparseInfo()
	{
		BT_PROFILE("clearSparseInfo=0");
		m_rowNonZeroElements1.resize(m_rows);
		m_colNonZeroElements.resize(m_cols);
		for (int i=0;i<m_rows;i++)
			m_rowNonZeroElements1[i].resize(0);
		for (int j=0;j<m_cols;j++)
			m_colNonZeroElements[j].resize(0);
	}

	void setZero()
	{
		{
			BT_PROFILE("storage=0");
			btSetZero(&m_storage[0],m_storage.size());
			//memset(&m_storage[0],0,sizeof(T)*m_storage.size());
			//for (int i=0;i<m_storage.size();i++)
	//			m_storage[i]=0;
		}
		{
			BT_PROFILE("clearSparseInfo=0");
			clearSparseInfo();
		}
	}
	
	void setIdentity()
	{
		btAssert(rows() == cols());
		
		setZero();
		for (int row=0;row<rows();row++)
		{
			setElem(row,row,1);
		}
	}

	

	void	printMatrix(const char* msg)
	{
		printf("%s ---------------------\n",msg);
		for (int i=0;i<rows();i++)
		{
			printf("\n");
			for (int j=0;j<cols();j++)
			{
				printf("%2.1f\t",(*this)(i,j));
			}
		}
		printf("\n---------------------\n");

	}
	void	printNumZeros(const char* msg)
	{
		printf("%s: ",msg);
		int numZeros = 0;
		for (int i=0;i<m_storage.size();i++)
			if (m_storage[i]==0)
				numZeros++;
		int total = m_cols*m_rows;
		int computedNonZero = total-numZeros;
		int nonZero = 0;
		for (int i=0;i<m_colNonZeroElements.size();i++)
			nonZero += m_colNonZeroElements[i].size();
		btAssert(computedNonZero==nonZero);
		if(computedNonZero!=nonZero)
		{
			printf("Error: computedNonZero=%d, but nonZero=%d\n",computedNonZero,nonZero);
		}
		//printf("%d numZeros out of %d (%f)\n",numZeros,m_cols*m_rows,numZeros/(m_cols*m_rows));
		printf("total %d, %d rows, %d cols, %d non-zeros (%f %)\n", total, rows(),cols(), nonZero,100.f*(T)nonZero/T(total));
	}
	/*
	void rowComputeNonZeroElements()
	{
		m_rowNonZeroElements1.resize(rows());
		for (int i=0;i<rows();i++)
		{
			m_rowNonZeroElements1[i].resize(0);
			for (int j=0;j<cols();j++)
			{
				if ((*this)(i,j)!=0.f)
				{
					m_rowNonZeroElements1[i].push_back(j);
				}
			}
		}
	}
	*/
	btMatrixX transpose() const
	{
		//transpose is optimized for sparse matrices
		btMatrixX tr(m_cols,m_rows);
		tr.setZero();
#if 0
		for (int i=0;i<m_cols;i++)
			for (int j=0;j<m_rows;j++)
			{
				T v = (*this)(j,i);
				if (v)
				{
					tr.setElem(i,j,v);
				}
			}
#else		
		for (int i=0;i<m_colNonZeroElements.size();i++)
			for (int h=0;h<m_colNonZeroElements[i].size();h++)
			{
				int j = m_colNonZeroElements[i][h];
				T v = (*this)(j,i);
				tr.setElem(i,j,v);
			}
#endif
		return tr;
	}

	void sortRowIndexArrays()
	{
		for (int i=0;i<m_rowNonZeroElements1[i].size();i++)
		{
			m_rowNonZeroElements1[i].quickSort(btIntSortPredicate());
		}
	}

	void sortColIndexArrays()
	{
		for (int i=0;i<m_colNonZeroElements[i].size();i++)
		{
			m_colNonZeroElements[i].quickSort(btIntSortPredicate());
		}
	}

	btMatrixX operator*(const btMatrixX& other)
	{
		//btMatrixX*btMatrixX implementation, optimized for sparse matrices
		btAssert(cols() == other.rows());

		btMatrixX res(rows(),other.cols());
		res.setZero();
//		BT_PROFILE("btMatrixX mul");
		for (int j=0; j < res.cols(); ++j)
		{
			//int numZero=other.m_colNonZeroElements[j].size();
			//if (numZero)
			{
				for (int i=0; i < res.rows(); ++i)
				//for (int g = 0;g<m_colNonZeroElements[j].size();g++)
				{
					T dotProd=0;
					T dotProd2=0;
					int waste=0,waste2=0;

					bool doubleWalk = false;
					if (doubleWalk)
					{
						int numRows = m_rowNonZeroElements1[i].size();
						int numOtherCols = other.m_colNonZeroElements[j].size();
						for (int ii=0;ii<numRows;ii++)
						{
							int vThis=m_rowNonZeroElements1[i][ii];
						}

						for (int ii=0;ii<numOtherCols;ii++)
						{
							int vOther = other.m_colNonZeroElements[j][ii];
						}


						int indexRow = 0;
						int indexOtherCol = 0;
						while (indexRow < numRows && indexOtherCol < numOtherCols)
						{
							int vThis=m_rowNonZeroElements1[i][indexRow];
							int vOther = other.m_colNonZeroElements[j][indexOtherCol];
							if (vOther==vThis)
							{
								dotProd += (*this)(i,vThis) * other(vThis,j);
							}
							if (vThis<vOther)
							{
								indexRow++;
							} else
							{
								indexOtherCol++;
							}
						}

					} else
					{
						bool useOtherCol = true;
						if (other.m_colNonZeroElements[j].size() <m_rowNonZeroElements1[i].size())
						{
						useOtherCol=true;
						}
						if (!useOtherCol )
						{
							for (int q=0;q<other.m_colNonZeroElements[j].size();q++)
							{
								int v = other.m_colNonZeroElements[j][q];
								T w = (*this)(i,v);
								if (w!=0.f)
								{
									dotProd+=w*other(v,j);
								}
						
							}
						}
						else
						{
							for (int q=0;q<m_rowNonZeroElements1[i].size();q++)
							{
								int v=m_rowNonZeroElements1[i][q];
								T w = (*this)(i,v);
								if (other(v,j)!=0.f)
								{
									dotProd+=w*other(v,j);	
								}
						
							}
						}
					}
					if (dotProd)
						res.setElem(i,j,dotProd);
				}
			}
		}
		return res;
	}

	// this assumes the 4th and 8th rows of B and C are zero.
	void multiplyAdd2_p8r (const btScalar *B, const btScalar *C,  int numRows,  int numRowsOther ,int row, int col)
	{
		const btScalar *bb = B;
		for ( int i = 0;i<numRows;i++)
		{
			const btScalar *cc = C;
			for ( int j = 0;j<numRowsOther;j++)
			{
				btScalar sum;
				sum  = bb[0]*cc[0];
				sum += bb[1]*cc[1];
				sum += bb[2]*cc[2];
				sum += bb[4]*cc[4];
				sum += bb[5]*cc[5];
				sum += bb[6]*cc[6];
				addElem(row+i,col+j,sum);
				cc += 8;
			}
			bb += 8;
		}
	}

	void multiply2_p8r (const btScalar *B, const btScalar *C,  int numRows,  int numRowsOther, int row, int col)
	{
		btAssert (numRows>0 && numRowsOther>0 && B && C);
		const btScalar *bb = B;
		for ( int i = 0;i<numRows;i++)
		{
			const btScalar *cc = C;
			for ( int j = 0;j<numRowsOther;j++)
			{
				btScalar sum;
				sum  = bb[0]*cc[0];
				sum += bb[1]*cc[1];
				sum += bb[2]*cc[2];
				sum += bb[4]*cc[4];
				sum += bb[5]*cc[5];
				sum += bb[6]*cc[6];
				setElem(row+i,col+j,sum);
				cc += 8;
			}
			bb += 8;
		}
	}
	
	void setSubMatrix(int rowstart,int colstart,int rowend,int colend,const T value)
	{
		int numRows = rowend+1-rowstart;
		int numCols = colend+1-colstart;
		
		for (int row=0;row<numRows;row++)
		{
			for (int col=0;col<numCols;col++)
			{
				setElem(rowstart+row,colstart+col,value);
			}
		}
	}
	
	void setSubMatrix(int rowstart,int colstart,int rowend,int colend,const btMatrixX& block)
	{
		btAssert(rowend+1-rowstart == block.rows());
		btAssert(colend+1-colstart == block.cols());
		for (int row=0;row<block.rows();row++)
		{
			for (int col=0;col<block.cols();col++)
			{
				setElem(rowstart+row,colstart+col,block(row,col));
			}
		}
	}
	void setSubMatrix(int rowstart,int colstart,int rowend,int colend,const btVectorX<T>& block)
	{
		btAssert(rowend+1-rowstart == block.rows());
		btAssert(colend+1-colstart == block.cols());
		for (int row=0;row<block.rows();row++)
		{
			for (int col=0;col<block.cols();col++)
			{
				setElem(rowstart+row,colstart+col,block[row]);
			}
		}
	}
	
	
	btMatrixX negative()
	{
		btMatrixX neg(rows(),cols());
		for (int i=0;i<m_colNonZeroElements.size();i++)
			for (int h=0;h<m_colNonZeroElements[i].size();h++)
			{
				int j = m_colNonZeroElements[i][h];
				T v = (*this)(i,j);
				neg.setElem(i,j,-v);
			}
		return neg;
	}

};



typedef btMatrixX<float> btMatrixXf;
typedef btVectorX<float> btVectorXf;

typedef btMatrixX<double> btMatrixXd;
typedef btVectorX<double> btVectorXd;


#ifdef BT_DEBUG_OSTREAM
template <typename T> 
std::ostream& operator<< (std::ostream& os, const btMatrixX<T>& mat)
	{
		
		os << " [";
		//printf("%s ---------------------\n",msg);
		for (int i=0;i<mat.rows();i++)
		{
			for (int j=0;j<mat.cols();j++)
			{
				os << std::setw(10) << mat(i,j);
			}
			if (i!=mat.rows()-1)
				os << std::endl << "  ";
		}
		os << " ]";
		//printf("\n---------------------\n");

		return os;
	}
template <typename T> 
std::ostream& operator<< (std::ostream& os, const btVectorX<T>& mat)
	{
		
		os << " [";
		//printf("%s ---------------------\n",msg);
		for (int i=0;i<mat.rows();i++)
		{
				os << std::setw(10) << mat[i];
			if (i!=mat.rows()-1)
				os << std::endl << "  ";
		}
		os << " ]";
		//printf("\n---------------------\n");

		return os;
	}

#endif //BT_DEBUG_OSTREAM


inline void setElem(btMatrixXd& mat, int row, int col, double val)
{
	mat.setElem(row,col,val);
}

inline void setElem(btMatrixXf& mat, int row, int col, float val)
{
	mat.setElem(row,col,val);
}

#ifdef BT_USE_DOUBLE_PRECISION
	#define btVectorXu btVectorXd
	#define btMatrixXu btMatrixXd
#else
	#define btVectorXu btVectorXf
	#define btMatrixXu btMatrixXf
#endif //BT_USE_DOUBLE_PRECISION



#endif//BT_MATRIX_H_H
