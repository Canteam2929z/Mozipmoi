#ifndef ZIPCORE_H
#define ZIPCORE_H

/* Giải nén toàn bộ file trong zipPath ra thư mục destRoot.
 * Trả về 0 nếu thành công, số âm nếu lỗi. */
int zip_extract(const char *zipPath, const char *destRoot);

#endif
