extern "C" int mz_compress(uchar *pDest, uint *pDest_len, const uchar *pSource, uint source_len);
extern "C" int mz_uncompress(uchar *pDest, uint *pDest_len, const uchar *pSource, uint source_len);
extern "C" int mz_compressBound(uint source_len);

namespace zip// namespace begin
{
	// Return status codes. MZ_PARAM_ERROR is non-standard.
	enum { MZ_OK = 0, MZ_STREAM_END = 1, MZ_NEED_DICT = 2, MZ_ERRNO = -1, MZ_STREAM_ERROR = -2, MZ_DATA_ERROR = -3, MZ_MEM_ERROR = -4, MZ_BUF_ERROR = -5, MZ_VERSION_ERROR = -6, MZ_PARAM_ERROR = -10000 };

	// Compression levels: 0-9 are the standard zlib-style levels, 10 is best possible compression (not zlib compatible, and may be very slow), MZ_DEFAULT_COMPRESSION=MZ_DEFAULT_LEVEL.
	enum { MZ_NO_COMPRESSION = 0, MZ_BEST_SPEED = 1, MZ_BEST_COMPRESSION = 9, MZ_UBER_COMPRESSION = 10, MZ_DEFAULT_LEVEL = 6, MZ_DEFAULT_COMPRESSION = -1 };

	uint deflateBound(uint source_len)
	{
	  // This is really over conservative. (And lame, but it's actually pretty tricky to compute a true upper bound given the way tdefl's blocking works.)
	  return max(128 + (source_len * 110) / 100, 128 + source_len + ((source_len / (31 * 1024)) + 1) * 5);
	}

	uchar* compress(const uchar *pSource, uint source_len, uint *pDest_len)
	{
		*pDest_len=mz_compressBound(source_len) +4;
		uchar *pDest = (uchar*)malloc(*pDest_len+4 );
		if((pDest)==0) error_stop("zip::compress Out of memory allocating %d MB",((*pDest_len) >> 20));

		int res = mz_compress(pDest+4, pDest_len, pSource, source_len);
		if( res != MZ_OK ) error_stop("zip::compress failed (%d)",res);
		pDest[0]=(source_len);
		pDest[1]=(source_len)>>8;
		pDest[2]=(source_len)>>16;
		pDest[3]=(source_len)>>24;

		(*pDest_len)=(*pDest_len)+4;

		printf("zipped %d to %d bytes\n",source_len,*pDest_len);
		return pDest;
	}

	uchar* uncompress(const uchar *pSource, uint source_len, uint *pDest_len)
	{
		*pDest_len=
			   (uint)	pSource[0]+
			 (((uint)	pSource[1])<<8)+
			 (((uint)	pSource[2])<<16)+
			 (((uint)	pSource[3])<<24);
			//=0)*pDest_len=deflateBound(source_len)  ;

		//printf("source_len %d ", (uint)source_len );
		//printf("pDest_len %d \n", (uint)*pDest_len );

		uchar *pDest = (uchar*)malloc((*pDest_len) );
		if((pDest)==0) error_stop("zip::uncompress Out of memory allocating %d MB",((*pDest_len) >> 20));
		int res = mz_uncompress(pDest, pDest_len, &pSource[4], source_len-4);
		if( res != MZ_OK ) error_stop("zip::uncompress failed (%d)",res);

		return pDest;
	}
}
