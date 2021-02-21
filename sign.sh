arm-none-eabi-objcopy -O binary ../freertos-tfm-fwu/freertos_kernel/portable/ThirdParty/GCC/ARM_CM33_TFM/build/RTOSDemo.axf RTOSDemo.bin
dir=$(pwd)
cd $dir/cmake_build/lib/ext/mcuboot-src/scripts && /usr/bin/python3 $dir/bl2/ext/mcuboot/scripts/wrapper/wrapper.py -v 0.0.0 --layout $dir/cmake_build/bl2/ext/mcuboot/CMakeFiles/signing_layout_ns.dir/signing_layout_ns.o -k $dir/bl2/ext/mcuboot/root-RSA-3072_1.pem --public-key-format full --align 1 --pad --pad-header -H 0x400 -s auto -d "(1, 0.0.0+0)" $dir/RTOSDemo.bin --overwrite-only $dir/RTOSDemo-signed.bin
