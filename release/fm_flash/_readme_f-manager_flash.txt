** Installing F256 f/manager in Flash **

1. unzip the f/manager archive to your local Windows/Linux/Mac computer.

2. copy the files in the f/manager archive into the folder with all your firmware files in it. Typically, this would include the following files:
	3f.bin
	3e.bin
	3d.bin
	3c.bin
	3b.bin
	docs_superbasic4.bin
	docs_superbasic3.bin
	docs_superbasic2.bin
	docs_superbasic1.bin
	dos.bin
	help.bin
	lockout.bin
	xdev.bin
	pexec.bin
(alternatively, you can copy these into your f/manager folder.)

3. Decide if you want f/manager to start up by default on your F256, or if you want SuperBASIC (or DOS) to be active on startup. 
   A. To have SuperBASIC on startup, use the bulk_fm_all_firmware.csv file.
   B. To have DOS on startup, use the bulk_fm_all_firmware.csv file, but edit it so that DOS comes before SuperBASIC.
   C. To have f/manager on startup, use the bulk_fm_first.csv file. After the first install, you may want to use bulk_fm_first_minimum.csv to save time.

4. With your F256 on, and connected via USB debug port to your modern computer, run the python FnxManager program using the bulk upload option (keeping in mind your choice from step 3. e.g.:
	python3 $FOENIXMGR/FoenixMgr/fnxmgr.py --flash-bulk bulk_fm_first.csv
	

That's it! If you chose to have f/manager in the first slot, when your F256 restarts, f/manager will start up automatically. Otherwise, you can call it from SuperBASIC with "/fm" or from DOS with "fm". 


** How it works **

How can you run a .pgZ file from flash, you ask? 

Through the magic of dwsJason and pexec. There is a compact version of pexec built into the first f/manager 8k chunk. When you call up f/manager by typing "fm" or by virtue of it being the lowest named program, it is actually pexec the runs. It "loads" the pgZ from flash, moving it to RAM as if it was loading it from disk. Pretty cool, eh? Magic!