#ifndef fl_lapackproto_single_h
#define fl_lapackproto_single_h


extern "C"
{
  void ssyev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   float        a[],
			   const int &  lda,
			   float        w[],
			   float        work[],
			   const int &  lwork,
			   int &        info);

  void sgelss_ (const int &   m,
				const int &   n,
				const int &   nrhs,
				float         a[],
				const int &   lda,
				float         b[],
				const int &   ldb,
				float         s[],
				const float & rcond,
				int &         rank,
                float         work[],
				const int &   lwork,
				int &         info);

  void sgesvd_ (const char & jobu,
				const char & jobvt,
				const int &  m,
				const int &  n,
				float        a[],
				const int &  lda,
				float        s[],
				float        u[],
				const int &  ldu,
				float        vt[],
				const int &  ldvt,
				float        work[],
				const int &  lwork,
				int &        info);

  void sgeev_ (const char & jobvl,
			   const char & jobvr,
			   const int &  n,
			   float        a[],
			   const int &  lda,
			   float        wr[],
			   float        wi[],
			   float        vl[],
			   const int &  ldvl,
			   float        vr[],
			   const int &  ldvr,
			   float        work[],
			   const int &  lwork,
			   int &        info);

  void sgetrf_ (const int & m,
				const int & n,
				float       a[],
				const int & lda,
				int         ipiv[],
				int &       info);

  void sgetri_ (const int & n,
				float       a[],
				const int & lda,
				int         ipiv[],
				float       work[],
				const int & lwork,
				int &       info);
}


#endif
