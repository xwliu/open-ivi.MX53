
Method 1: Build into android rootfs image.

Step 1: Modify config file.

Modify the value of EXCLUDED_CODEC_BUILD to true in ~/myandroid/device/fsl/imx5x/BoardConfigCommon.mk

Step 2: Install codec library.

	tar xzvf vpu_fw_mx5x.tar.gz
	cp -r vpu_fw_mx5x/ ~/myandroid/device/fsl/proprietary
	cd ~/myandroid
	source build/envsetup.sh
	make PRODUCT-imx51_bbg-eng (for imx51 BBG, or make PRODUCT-  imx53_evk-eng for imx53 EVK)


Method 2: Install into existed android rootfs.

Step 1: Install codec library.

For NFS rootfs:

	tar xzvf vpu_fw_mx5x.tar.gz   
	cd vpu_fw_mx5x/
	cp vpu_fw_mx5x.bin <your_nfs_rootfs>/system/lib/firmware/vpu
	 
For SD rootfs:
	  
	tar xzvf vpu_fw_mx5x.tar.gz
	cd  vpu_fw_mx5x/
	mount -t ext3 -o loop system.img /mnt
	cp vpu_fw_mx5x.bin /mnt/lib/firmware/vpu

