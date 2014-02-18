
Method 1: Build into android rootfs image.

Step 1: Install codec library.

	tar xzvf fsl_ac3_dec.tar.gz
	cp -r fsl_ac3_dec/ ~/myandroid/device/fsl/proprietary
	cd ~/myandroid
	source build/envsetup.sh
	make PRODUCT-imx51_bbg-eng (for imx51 BBG, or make PRODUCT-  imx53_evk-eng for imx53 EVK)


Method 2: Install into existed android rootfs.

Step 1: Install codec library.

For NFS rootfs:

	tar xzvf fsl_ac3_dec.tar.gz   
	cd fsl_ac3_dec/
	cp *.so <your_nfs_rootfs>/system/lib
	 
For SD rootfs:
	  
	tar xzvf fsl_ac3_dec.tar.gz
	cd  fsl_ac3_dec/
	mount -t ext3 -o loop system.img /mnt
	cp *.so /mnt/lib

