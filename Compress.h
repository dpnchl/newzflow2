#pragma once

namespace Compress {
	bool UnZip(const CString& zipfile, const CString& dstPath, float* progress);
	bool UnRar(const CString& rarfile, const CString& dstPath, float* progress);
};
