# UnzipDylib

Dylib tu dong tim `zip1.zip` .. `zip9.zip` trong app bundle (Payload) va giai nen tung file ra thu muc **Documents** cua app.
Thuong dung de tam thoi "giai phong" du lieu cua mot app duoc re-sign/inject IPA.

## Cau truc
- `Tweak.xm`   : entry point, tim zip1..zip9.zip trong bundle va giai nen ra Documents khi dylib duoc nap.
- `zipcore.c/h`: bo giai nen ZIP toi gian (zlib), ho tro method 0 (stored) & 8 (deflate).
- `Makefile`   : Theos tweak build -> **.dylib**.
- `control`    : metadata goi.
- `build.sh`   : build nhanh tren macOS.

## Build (tren macOS co Theos)
```bash
export THEOS=~/theos
bash -c "$(curl -fsSL https://raw.githubusercontent.com/theos/theos/master/bin/install-theos)"  # neu chua co
./build.sh
```
Ket qua: file `.dylib` nam trong `.theos/obj/`.

## Test phan loi ZIP tren Linux (khong can mac)
```bash
gcc -O2 -o test_zipcore test_zipcore.c zipcore.c -lz
./test_zipcore <file.zip> <thu-muc-dich>
```

## Cach inject vao IPA
1. Build lay `UnzipDylib.dylib`.
2. Dung `optool` gan load command vao Mach-O cua app:
```bash
optool install -c load -p "@executable_path/UnzipDylib.dylib" -t Payload/AppName.app/AppName
```
3. Copy `UnzipDylib.dylib` vao `Payload/AppName.app/`.
4. Neu app da ky: re-sign bang `codesign` / `ldid`.
5. Dat file can giai nen (vd `data.zip`) vao trong `Payload/AppName.app/` va zip lai thanh IPA.

## Ghi chu
- Dylib tim lan luot `zip1.zip` den `zip9.zip` trong bundle (quet ca 1 cap thu muc con) va giai nen ra Documents.
- Ho tro gioi han: khong ho tro ZIP64 (file < 4GB, so entry < 65535).
- `notify_post("com.user.unzipdylib.done")` phat tin hieu khi xong (co the lang nghe de debug).
