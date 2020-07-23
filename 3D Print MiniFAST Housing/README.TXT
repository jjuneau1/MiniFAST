--------------------------------------------------
Author: Savva Morozov
Revision: 1.0, July 27th 2019
Affiliation: RNEL, Rice University
--------------------------------------------------
The following details Miniscope 3D-printable devices that I designed for RNEL and Jill Juneau
--------------------------------------------------
General Notes:
--------------------------------------------------
.stl and .stp files are provided. Use .stl for 3D printing, and .stp for editting 3D models using CAD software of your choice. 

.form files provided for the full miniscope, a batch of baseplates, and a batch of housings.
--------------------------------------------------
--------------------------------------------------
Overview
--------------------------------------------------
	• These folders contain miniscope related 3D models and designs that I created in 2018-19 for Dr Kemere’s Lab. In particular, the miniscope folders has the redesigned miniscope files. 
	• The goals of the developed design were to:
		• Make the process of attaching the miniscope to mice’ head easier, less time consuming, and less handling dependent. This is achieved using a twist-lock attachment mechanism.
		• Allow the use of a new better camera. Specifically, increasing the field of view, accounting for camera’s size & heat dissipation, allowing for easier/faster vertical adjustment. 
--------------------------------------------------
--------------------------------------------------
How to print and make the miniscope on Form2.
--------------------------------------------------
--------------------------------------------------
--------------------------------------------------
Part 1: Download the files, print the scope.
--------------------------------------------------
	• Go to Miniscope/PRINT ME folder, and open the .form file using PreForm V3 software. This file contains each individual part of one copy of a miniscope, setup and ready to go, using the settings that worked best for me for the Form2 printer. 
	• The parts MUST be printed in Black resin, 0.025mm resolution. Change the resin version (V4,V5,V6) according to your setup (respective menu is on the right).
	• Hit the orange button on the left. Select your printer. Upload the file to the printer. 
	• Ensure that the resin you have in the printer is Black. Start the printing process on the printer.

	Extra: add multiple copies of the miniscope to the print file.
		• Select every part (parts would appear in blue if selected).
		• Press Layout icon (on the left, just above the orange circle button).
		• Adjust the Duplicate Count value and press Create.
		• Reposition the parts so that they have a common ground raft support plane.
		• You might want to get rid of a spare cap closer. I mean, you really only need one.

	Extra: tweak with supports, rotation of individual parts. 
		• This requires more extensive knowledge of PreForm, and more detailed understanding of the models. See more info below.
--------------------------------------------------
--------------------------------------------------
Part 2: Post Processing / Assembly.
--------------------------------------------------
	• Put the gloves on. Do not touch Form2 resin with bare hands! 
	• Carefully take the parts off the print-bed.
	• Wash them in IPA (isopropyl alcohol) for ~10 minutes (FormWash works best).
	• Wait till the parts are dry. Examine the parts for any left-over resin - leftovers would be shiny and sticky. Use IPA spray to get rid of it as much of it as you can. Make sure to check the cavities.
	• Wait till the parts are dry and expose them to UV light for 30-50 minutes. If you have a UV Chamber - 30 mins is enough; mint want to do more if using some other setup.


	• Carefully remove the supports using a sharp exacto-knife and a micro cutter. Be careful to not chip off some fine details, like the twist lock mechanism!!   
		• TIP: first use a micro cutter to cut off the supports from the raft, then use the exacto-knife to cut the supports from each individual part. 
	• Use the exacto-knife to score and scratch the bottom and the sides of the base plate. This is important for the surgery - it would allow dental epoxy to better adhere to the base plate.

	• Glue the magnets to the parts using super glue; pay attention to the relative orientation of the magnets! Refer to an image in google drive for proper orientation.   
		•   First put the magnet into the cavity; make sure that it’s flush against the surface.    
		•   Take liquid super glue and a disposable pipette; cut the tip of the pipette off, slide it on top of the tip of the glue bottle - this would give you finer control over the volume of glue you are depositing.   
		•  Carefully put a drop or two of glue on top of the magnet. Wait a few seconds, then touch the magnet with a Kim wipe: the Kim wipe should soak all the excess super glue. Wait for a few a minute or so.   
		• You are going to have dry super glue all over the surface of the part. Use an exacto-knife to scratch this super glue off. This is important, as otherwise the locking mechanism wouldn’t work as nicely.

	• You will need #0-80 x 3/16’’ screws and #0-80 nuts for the clamp of the top part. I would also advise attaching the top to the main part with tack once you figure out a permanent vertical position of the top.

	• That’s it!

--------------------------------------------------
--------------------------------------------------
More on printing
--------------------------------------------------
The Miniscope
	About rotational settings.  For those who want to tweak with PreForm and .stl files on their own. The following are rotational settings that I found to work best with my the current files (X/Y are axis; angles are given in degrees; order of rotations matters)

	  Top with clamp:      Y: -90       X: 5 
	 Main:                X: -12       Y: 15  
	Base plate:          X: 45        Y: -20	 
	Cover:               X: -45 
	Cap:                 X: -45       Y: 15    
	Cap Closer:          X: -30  
	Lens Holder:         Y: 180       X: 10 
--------------------------------------------------
About supports settings. 
	 I found that 0.6mm point size worked best for me - the prints haven’t failed once.  

	Make sure to uncheck the “Inner Supports” checkmark! Inner supports are unnecessary and would only make it more difficult for you to take off the supports.















