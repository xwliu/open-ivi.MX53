
Method 1: Build into android rootfs image.

Step 1: Modify config file.

Enable ASF parser in ~/myandroid/device/fsl/imx5x/init.rc

	setprop ro.FSL_ASF_PARSER 1

Add below text entry in ~/myandroid/device/fsl/proprietary/omx/registry/component_register.

@
component_name=OMX.Freescale.std.video_decoder.wmv9.hw-based;
library_path=lib_omx_vpu_dec_v2_arm11_elinux.so;
component_entry_function=VpuDecoderInit;
component_role=video_decoder.wmv9;
role_priority=3;
$

Step 2: Install codec library.

	tar xzvf fsl_ms_codec.tar.gz
	cp -r fsl_ms_codec/ ~/myandroid/device/fsl/proprietary
	cd ~/myandroid
	source build/envsetup.sh
	make PRODUCT-imx51_bbg-eng (for imx51 BBG, or make PRODUCT-  imx53_evk-eng for imx53 EVK)


Method 2: Install into existed android rootfs.

Step 1: Modify config file.

Enable ASF parser in <your_nfs_rootfs>/init.rc.

	setprop ro.FSL_ASF_PARSER 1

Add below text entry in <your_nfs_rootfs>/system/etc/component_register.

@
component_name=OMX.Freescale.std.video_decoder.wmv9.hw-based;
library_path=lib_omx_vpu_v2_arm11_elinux.so;
component_entry_function=Vpu_ComponentInit;
component_role=video_decoder.wmv9;
role_priority=3;
$

Step 2: Install codec library.

For NFS rootfs:

	tar xzvf fsl_ms_codec.tar.gz   
	cd fsl_ms_codec/
	cp *.so <your_nfs_rootfs>/system/lib
	 
For SD rootfs:
	  
	tar xzvf fsl_ms_codec.tar.gz
	cd  fsl_ms_codec/
	mount -t ext3 -o loop system.img /mnt
	cp *.so /mnt/lib

