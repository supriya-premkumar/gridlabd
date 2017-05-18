import os;
gld_clock = os.getenv('clock');
gld_stopat = os.getenv('stopat');
if ( gld_stopat == gld_clock ) :
	input("STOPAT {}: ".format(gld_clock));
else :
	print("[{}]: waiting for {}".format(gld_clock,gld_stopat));
