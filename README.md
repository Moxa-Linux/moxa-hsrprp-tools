# moxa-hsrprp-tools

MOXA HSR/PRP card utility base on SMBUS to query FPGA related register.

1. Install libi2c-dev
```
	# sudo apt-get install libi2c-dev
```

2. Compiler and install Moxa HSR/PRP card utility
```
	# make install
	mxhsrprpd	- Configure hsr/prp mode, collect Ethernet counters, link status, link speed
	mxprpsuper	- Send PRP/HSR supervison frame.
	mxprpinfo	- Get current hsr/prp mode, collect Ethernet counters, link status, link speed. Depends on mxhsrprpd
	chk-mx-prp-card	- Report on board Moxa HSR/PRP card interface name
	mxprpalarm	- mxhsrprpd will execute this when link status change. This script can be modify by customer.
```

3. Verify main board SMBUS host driver is exist. The following command is Intel x86 platform
```
	lsmod | grep i2c_i801
```

4. Troubleshooting for i2c_i801 driver.
```
	a. for DA-720/DA682B, the driver is built-in kernel 3.19 or later
	   for DA-820, the driver is built-in kernel 3.0 or later
           for DA-820C, the driver is built-in kernel 4.0 or later
	b. ACPI conflict with SMBUS issue
	   1) edit /etc/default/grub and insert acpi_enforce_resources=lax into the parameter string of GRUB_CMDLINE_LINUX,
	   e. g. GRUB_CMDLINE_LINUX='acpi_enforce_resources=lax'
	   2) then run update-grub and reboot.
```

5. Verify I2C-dev bus number, the following result is Intel x86 platform.
```
	# cat /sys/class/i2c-dev/i2c-0/name
	SMBus I801 adapter at b000
```

6. Launch the mxhsrprpd manually
```
	# mxhsrprpd -t 2
```

7. Run the mxhsrprpd automatically at booting
```
	For Debian 7 system:

	root@Moxa:/home/mxhsrprp# cp -a fakeroot/etc/init.d/mx_prp.sh /etc/init.d/
	root@Moxa:/home/mxhsrprp# insserv -d mx_prp.sh

	For Debian 8/9/later system:
	root@Moxa:/home/mxhsrprp# systemctl daemon-reload && systemctl start mx_hsrprp.service

	For Ubuntu:

	root@Moxa:/home/mxhsrprp# cp -a fakeroot/etc/init.d/mx_prp.sh /etc/init.d/
	root@Moxa:/home/mxhsrprp# update-rc.d mx_prp.sh defaults

	For Redhat Enterprise:

	root@Moxa:/home/mxhsrprp# chkconfig --levels 2345 mx_prp.sh on
```
