32-bit efi for Asus T100
========================

To generate this file yourself, do the following on any computer:

$ sudo apt-get install git bison libopts25 libselinux1-dev autogen m4 autoconf help2man libopts25-dev flex libfont-freetype-perl automake autotools-dev libfreetype6-dev texinfo

$ git clone git://git.savannah.gnu.org/grub.git

$ cd grub

$ ./autogen.sh

$ export EFI_ARCH=i386

$ ./configure --with-platform=efi --target=${EFI_ARCH} --program-prefix=""

$ make

$ cd grub-core

$ ../grub-mkimage -d . -o bootia32.efi -O i386-efi -p /boot/grub ntfs hfs appleldr boot cat efi_gop efi_uga elf fat hfsplus iso9660 linux keylayouts memdisk minicmd part_apple ext2 extcmd xfs xnu part_bsd part_gpt search search_fs_file chain btrfs loadbios loadenv lvm minix minix2 reiserfs memrw mmap msdospart scsi loopback normal configfile gzio all_video efi_gop efi_uga  gfxterm gettext echo boot chain eval

