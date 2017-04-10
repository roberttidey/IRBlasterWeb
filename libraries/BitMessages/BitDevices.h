// BitDevices.h
//
// Defines devices to be used with bit bang messaging
// Define the buttons / codes as pairs of strings (name:hex code)
// The data structure then defines basic pulse coding, bit length etc

#define NUMBER_DEVICES 10

//lgtv, yamaha, youview fully defined
//nec,panasonic,jvc,samsung,sony,rc51,rc61 untested placeholders and need button codes

char* lgTVMsgs[] = {
	"ONOFF","20DF10EF",
	"HELP","20DF5EA1",
	"RATIO","20DF9E61",
	"INPUT","20DFD02F",
	"TVRAD","20DF0FF0",
	"1","20DF8877",
	"2","20DF48B7",
	"3","20DFC837",
	"4","20DF28D7",
	"5","20DFA857",
	"6","20DF6897",
	"7","20DFE817",
	"8","20DF18E7",
	"9","20DF9867",
	"KEYLIST","20DFCA35",
	"0","20DF08F7",
	"KEYVIEW","20DF58A7",
	"VOLUP","20DF40BF",
	"FAV","20DF7887",
	"PROGUP","20DF00FF",
	"INFO","20DF55AA",
	"VOLDOWN","20DFC03F",
	"MUTE","20DF906F",
	"PROGDOWN","20DF807F",
	"SETTINGS","20DFC23D",
	"HOME","20DF3EC1",
	"MYAPPS","206FA15E",
	"UP","20DF02FD",
	"LEFT","20DFE01F",
	"OK","20DF22DD",
	"RIGHT","20DF609F",
	"DOWN","20DF827D",
	"BACK","20DF14EB",
	"GUIDE","20DFD52A",
	"EXIT","20DFDA25",
	"RED","20DF4EB1",
	"GREEN","20DF8E71",
	"YELLOW","20DFC639",
	"BLUE","20DF8679",
	"TEXT","20DF04FB",
	"TOPT","20DF847B",
	"QMENU","20DFA25D",
	"STOP","20DF8D72",
	"PLAY","20DF0DF2",
	"PAUSE","20DF5DA2",
	"FASTBACK","20DFF10E",
	"FASTFORWARD","20DF718E",
	"RECORD","20DFBD42",
	"SAVING","20DFA956",
	"AD ","20DF8976",
	"APP","20DFF906",
	NULL
};


char* yamahaAVMsgs[] = {
	"ONOFF","7E8154AB",
	"HDMI1","5EA1E21C",
	"HDMI2","5EA152AC",
	"HDMI3","5EA1B24C",
	"HDMI4","5EA10AF4",
	"AV1","5EA1CA34",
	"AV2","5EA16A94",
	"AV3","5EA19A64",
	"HDMI5","5EA10EF0",
	"AV4","5EA13AC4",
	"AV5","5EA1FA04",
	"AV6","5EA146B8",
	"SINGLESTAR","5EA116E8",
	"AUX","5EA1AA55",
	"USB","FE804EB0",
	"NET","FE80FC03",
	"AUDIO","5EA1A658",
	"FM","FE801AE4",
	"AM","FE80AA54",
	"PROGUP","FE80DA24",
	"TUNINGUP","FE808678",
	"INFO","5EA1E41A",
	"MEMORY","FE80E618",
	"PROGDOWN","FE807A84",
	"TUNINGDOWN","FE8026D8",
	"MOVIE","5EA111EE",
	"MUSIC","5EA1916E",
	"SURDECODE","5EA1B14E",
	"STRAIGHT","5EA16A95",
	"BEEP","5EA10CF3",
	"ENHANCER","5EA129D6",
	"DIRECT","5EA1BB44",
	"SCENEDVD","5EA100FE",
	"SCENETV","5EA1C03E",
	"SCENENET","5EA1609E",
	"SCENERADIO","5EA1906E",
	"SETUP","5EA121DE",
	"UP","5EA1B946",
	"OPTION","5EA1D628",
	"VOLUP","5EA158A7",
	"LEFT","5EA1F906",
	"OK","5EA17B84",
	"RIGHT","5EA17986",
	"RETURN","5EA155AA",
	"DOWN","5EA139C6",
	"DISPLAY","FE8006F9",
	"VOLDOWN","5EA1D827",
	"TOPMENU","5EA105FB",
	"POPUPMENU","5EA125DB",
	"MUTE","5EA138C7",
	"MODE","FE806699",
	"STOP","FE809669",
	"PAUSE","FE80E619",
	"PLAY","FE8016E9",
	"FASTBACK","FE8056A9",
	"FASTFORWARD","FE80D629",
	"BACKHOME","FE8036C9",
	"FORWARDEND","FE80B649",
	"1","FE808A75",
	"2","FE804AB5",
	"3","FE80CA35",
	"4","FE802AD5",
	"5","FE80AA55",
	"6","FE806A95",
	"7","FE80EA15",
	"8","FE801AE5",
	"9","FE809A65",
	"0","FE805AA5",
	"PLUS10","FE80DA25",
	"KEYENT","FE803AC5",
	NULL
};

char* youviewMsgs[] = {
	"ONOFF","000800FF",
	"ZOOM","0008807F",
	"TEXT","00087689",
	"HELP","0008E817",
	"BTPLAYER","0008D22D",
	"MENU","0008708F",
	"RED","000838C7",
	"GREEN","0008B847",
	"YELLOW","000858A7",
	"BLUE","00087887",
	"VOLUP","0008F807",
	"INFO","0008C23D",
	"PROGUP","000808F7",
	"MUTE","000818E7",
	"LEFT","000848B7",
	"UP","00088877",
	"OK","0008C837",
	"DOWN","0008A857",
	"RIGHT","000828D7",
	"VOLDOWN","000802FD",
	"BACK","0008827D",
	"PROGDOWN","0008F00F",
	"FASTBACK","0008A659",
	"PLAY","000806F9",
	"PAUSE","000846B9",
	"FASTFORWARD","000826D9",
	"SKIPBACK","00086699",
	"RECORD","00088679",
	"STOP","0008C639",
	"SKIPFORWARD","0008E619",
	"GUIDE","0008D827",
	"SEARCH","0008E21D",
	"CLOSE","00086897",
	"1","0008C03F",
	"2","000820DF",
	"3","0008A05F",
	"4","0008609F",
	"5","0008E01F",
	"6","000810EF",
	"7","0008906F",
	"8","000850AF",
	"9","0008D02F",
	"KEYADDUP","000822DD",
	"0","000830CF",
	"KEYDELETE","0008629D",
	NULL
};

char* necMsgs[] = {
	NULL
};

char* panasonicMsgs[] = {
	NULL
};

char* jvcMsgs[] = {
	NULL
};

char* samsungMsgs[] = {
	NULL
};

char* sonyMsgs[] = {
	NULL
};

char* virginCableMsgs[] = {
"ONOFF","D460",
"HOME","5478",
"TV","5498",
"GUIDE","1490",
"UP","5480",
"INFO","9578",
"BACK","D510",
"LEFT","14A8",
"OK","54B8",
"RIGHT","14B0",
"TEXT","5550",
"DOWN","1488",
"SUBTITLES","D4D0",
"MYSHOWS","94A8",
"PROGUP","D500",
"PROGDOWN","D508",
"DISLIKE","94B0",
"RECORD","D5B8",
"LIKE","94B8",
"PLAY","D580",
"FASTBACK","9590",
"PAUSE","D498",
"FASTFORWARD","95A0",
"STOP","D5B0",
"SKIPBACK","9550",
"SLOW","D4C0",
"SKIPFORWARD","9558",
"RED","5558",
"GREEN","1560",
"YELLOW","5568",
"BLUE","1570",
"1","9408",
"2","D410",
"3","9418",
"4","D420",
"5","9428",
"6","D430",
"7","9438",
"8","D440",
"9","9448",
"KEYCLEAR","D4C8",
"0","9400",
"KEYLASTCH","D4A0",
NULL};

char* philipsDVDMsgs[] = {
"ONOFF","DF670",
"MENU","E065C",
"DISPLAY","DF7E0",
"UP","E074E",
"LEFT","DF74A",
"OK","E0746",
"RIGHT","DF748",
"DOWN","E074C",
"TITLE","DF6F8",
"SETUP","E06FA",
"PREV","DF7BC",
"NEXT","E07BE",
"PLAYPAUSE","DF7A6",
"STOP","E079C",
"USB","DF702",
"1","E07FC",
"2","DF7FA",
"3","E07F8",
"4","DF7F6",
"5","E07F4",
"6","DF7F2",
"7","E07F0",
"8","DF7EE",
"9","E07EC",
"SUBTITLE","DF768",
"0","E07FE",
"AUDIO","DF762",
"ZOOM","E0610",
"REPEAT","DF7C4",
"REPEATAB","E0788",
NULL
};

deviceData devices[NUMBER_DEVICES] = {
	"lgTV",
	"H9000,L4500", //header
	NULL, //trailer
	"H550,L550",
	"H550,L1600",
	38000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	lgTVMsgs,
	
	"yamahaAV",
	"H9000,L4500", //header
	NULL, //trailer
	"H550,L550",
	"H550,L1600",
	38000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	yamahaAVMsgs,
	
	"youview",
	"H9000,L4500", //header
	NULL, //trailer
	"H550,L550",
	"H550,L1600",
	38000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	youviewMsgs,
	
	"nec",
	"H9000,L4500", //header
	NULL, //trailer
	"H550,L550",
	"H550,L1600",
	38000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	necMsgs,
	
	"panasonic",
	"H3500,L1750", //header
	NULL, //trailer
	"H500,L400",
	"H500,L1250",
	35000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	panasonicMsgs,
	
	"jvc",
	"H8000,L4000", //header
	NULL, //trailer
	"H600,L550",
	"H600,L1600",
	38000, //frequency
	0, //Special handling
	75, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	jvcMsgs,
	
	"samsung",
	"H4500,L4500", //header
	NULL, //trailer
	"H600,L600",
	"H600,L1700",
	38000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	1, // minimum repeat
	0, //toggle
	samsungMsgs,
	
	"sony",
	"H2400,L600", //header
	NULL, //trailer
	"H650,L600",
	"H1250,L600",
	40000, //frequency
	0, //Special handling
	100, //repeat gap mSec
	33, //bit count
	2, // minimum repeat
	0, //toggle
	sonyMsgs,
	
	"virginCable",
	"H890,L890,H890", //header
	"L890", //trailer
	"L890,H890",
	"H890,L890",
	36000, //frequency
	0, //Special handling
	45, //repeat gap mSec
	13, //bit count
	1, // minimum repeat
	0, //toggle
	virginCableMsgs,

	"philipsDVD",
	"H2700,L890", //header
	"L450", //trailer
	"L450,H450",
	"H450,L450",
	36000, //frequency
	1, //Special handling
	45, //repeat gap mSec
	19, //bit count
	1, // minimum repeat
	0, //toggle
	philipsDVDMsgs

};

