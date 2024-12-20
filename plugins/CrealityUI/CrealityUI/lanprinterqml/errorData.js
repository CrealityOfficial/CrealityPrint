const errorJson = {
    1:{
        "code":"1",
        "level":2,//1:严重 2：普通 3：特殊
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Motor drive abnormality"),
        "supportVersion": 800
    },
    2:{
        "code":"2",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("internal error"),
        "supportVersion": 800
    },
    3:{
        "code":"3",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Communication abnormality"),
        "supportVersion": 800
    },
    4:{
        "code":"4",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Not heating as expected"),
        "supportVersion": 800
    },
    209:{
        "code":"209",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Abnormal hot bed temperature"),
        "supportVersion": 800
    },
    6:{
        "code":"6",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Extruder abnormality"),
        "supportVersion": 800
    },
    7:{
        "code":"7",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Print file coordinates are abnormal"),
        "supportVersion": 800
    },
    101:{
        "code":"101",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("A print quality problem has been detected and printing is paused"),
        "supportVersion": 800
    },
    100:{
        "code":"100",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("An unknown error was detected in printing"),
        "supportVersion": 800
    },
    201:{
        "code":"201",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Abnormal temperature of the temperature chamber"),
        "supportVersion": 800
    },
    9:{
        "code":"9",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Chatter optimization sensor abnormality"),
        "supportVersion": 800
    },
    10:{
        "code":"10",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Fan abnormality"),
        "supportVersion": 800
    },
    204:{
        "code":"204",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Fan abnormality"),
        "supportVersion": 800
    },
    205:{
        "code":"205",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("network anomaly"),
        "supportVersion": 800
    },
    206:{
        "code":"206",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("AI detected print quality issues"),
        "supportVersion": 800
    },
    207:{
        "code":"207",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("z-Touch abnormality"),
        "supportVersion": 800
    },
    208:{
        "code":"208",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("File anomaly"),
        "supportVersion": 800
    },
    500:{
        "code":"500",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Unknown exception"),
        "supportVersion": 800
    },
    800:{
        "code":"800",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Axis movement error"),
        "supportVersion": 800
    },
    801:{
        "code":"801",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("Machine preparation"),
        "supportVersion": 800
    },
    20000:{
        "code":"20000",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("20000"),
        "supportVersion": 800
    },
    20010:{
        "code":"20010",
        "level":2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":qsTr("20010"),
        "supportVersion": 800
    },
    101: {
        "code": "AC0101",
        "level": 1, //1:严重 2：普通 3：特殊
        "apply":["K1C","K1MAX", "K2Plus"],
        "content": cxTr("AC0101"),
        "supportVersion": 830
    },
    103:{
        "code": "AC0103",
        "level": 1,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0103"),
        "supportVersion": 830
    },
    104:{
        "code": "AC0104",
        "level": 1,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0104"),
        "supportVersion": 830
    },
    117: {
        "code": "AC0117",
        "level": 1,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0117"),
        "supportVersion": 830
    },
    500:{
        "code": "AC0500",
        "level": 2,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0500"),
        "supportVersion": 830
    },
    503:{
        "code": "AC0503",
        "level": 2,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0503"),
        "supportVersion": 830
    },
    509:{
        "code": "AC0509",
        "level": 2,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0509"),
        "supportVersion": 830
    },
    510:{
        "code": "AC0510",
        "level": 2,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0510"),
        "supportVersion": 830
    },
    523:{
        "code": "AC0523",
        "level": 2,
        "apply":["K1C","K1MAX", "K2Plus"],
        "content":cxTr("AC0523"),
        "supportVersion": 830
    },
    504:{
        "code": "AL0504",
        "level": 2,
        "apply":["K1MAX", "K2Plus"],
        "content":cxTr("AL0504"),
        "supportVersion": 830
    },
    505:{
        "code": "AL0505",
        "level": 2,
        "apply":["K1MAX"],
        "content":cxTr("AL0505"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-max/troubleshooting/ai-lidar-does-not-detect"
    },
    506:{
        "code": "AL0506",
        "level": 2,
        "apply":["K1MAX"],
        "content":cxTr("AL0506"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-series-general-documents/506-error-trouble-shooting"
    },
    507:{
        "code": "AL0507",
        "level": 2,
        "apply":["K1MAX"],
        "content":cxTr("AL0507"),
        "supportVersion": 830
    },
    110:{
        "code": "BM0110",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("BM0110"),
        "supportVersion": 830
    },
    111:{
        "code": "BM0111",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("BM0111"),
        "supportVersion": 830
    },
    112:{
        "code": "BM0112",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("BM0112"),
        "supportVersion": 830
    },
    2512:{
        "code": "BM2512",
        "level": 1,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("BM2512"),
        "supportVersion": 830
    },
    508:{
        "code": "BS0508",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("BS0508"),
        "supportVersion": 830
    },
    2721:{
        "code": "CA2721",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("CA2721"),
        "supportVersion": 830
    },
    2510:{
        "code": "CB2510",
        "level": 1,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CB2510"),
        "supportVersion": 830
    },
    2516:{
        "code": "CB2516",
        "level": 1,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CB2516"),
        "supportVersion": 830
    },
    2565:{
        "code": "CB2565",
        "level": 1,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CB2565"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-max/troubleshooting/2565-error-trouble-shooting"
    },
    502:{
        "code": "CF0502",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CF0502"),
        "supportVersion": 830
    },
    2536:{
        "code": "CL2536",
        "level": 1,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CL2536"),
        "supportVersion": 830
    },
    2537:{
        "code": "CL2537",
        "level": 1,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CL2537"),
        "supportVersion": 830
    },
    115:{
        "code": "CM0115",
        "level": 3,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CM0115"),
        "supportVersion": 830
    },
    2781:{
        "code": "CM2781",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CM2781"),
        "supportVersion": 830
    },
    2782:{
        "code": "CM2782",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CM2782"),
        "supportVersion": 830
    },
    2783:{
        "code": "CM2783",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CM2783"),
        "supportVersion": 830
    },
    2787:{
        "code": "CM2787",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CM2787"),
        "supportVersion": 830
    },
    2790:{
        "code": "CM2790",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CM2790"),
        "supportVersion": 830
    },
    2511:{
        "code": "CT2511",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CT2511"),
        "supportVersion": 830
    },
    2517:{
        "code": "CT2517",
        "level": 2,
        "apply":["K1","K1C","K1MAX", "K2Plus"],
        "content":cxTr("CT2517"),
        "supportVersion": 830
    },
    2566:{
        "code": "CX2566",
        "level": 1,
        "apply":["K1","K1C","K1MAX"],
        "content":cxTr("CX2566"),
        "supportVersion": 830
    },
    2573:{
        "code": "CX2573",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CX2573"),
        "supportVersion": 830
    },
    2585:{
        "code": "CX2585",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CX2585"),
        "supportVersion": 830
    },
    2567:{
        "code": "CY2567",
        "level": 1,
        "apply":["K1","K1C","K1MAX"],
        "content":cxTr("CY2567"),
        "supportVersion": 830
    },
    2577:{
        "code": "CY2577",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CY2577"),
        "supportVersion": 830
    },
	2581:{
        "code": "CY2581",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CY2581"),
        "supportVersion": 830
    },
    2586:{
        "code": "CY2586",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CY2586"),
        "supportVersion": 830
    },
    2568:{
        "code": "CZ2568",
        "level": 1,
        "apply":["K1","K1C","K1MAX"],
        "content":cxTr("CZ2568"),
        "supportVersion": 830
    },
    2581:{
        "code": "CZ2581",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CZ2581"),
        "supportVersion": 830
    },
    2587:{
        "code": "CZ2587",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CZ2587"),
        "supportVersion": 830
    },
    2588:{
        "code": "CZ2588",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("CZ2588"),
        "supportVersion": 830
    },
    2844:{
        "code": "FB2844",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FB2844"),
        "supportVersion": 830,
        "isCfs":true
    },
    2846:{
        "code": "FB2846",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FB2846"),
        "supportVersion": 830,
        "isCfs":true
    },
    2847:{
        "code":"FB2847",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FB2847"),
        "supportVersion": 830,
        "isCfs":true
    },
    2837:{
        "code":"FO2837",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FO2837'),
        "supportVersion": 830,
        "isCfs":true
    },
    2838:{
        "code":"FO2838",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FO2838'),
        "supportVersion": 830,
        "isCfs":true
    },
    2845:{
        "code":"FO2845",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FO2845'),
        "supportVersion": 830,
        "isCfs":true
    },

    2832:{
        "code":"FR2832",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FR2832'),
        "supportVersion": 830,
        "isCfs":true
    },
    2833:{
        "code":"FR2833",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FR2833'),
        "supportVersion": 830,
        "isCfs":true
    },
    2835:{
        "code":"FR2835",
        "level": 1,
        "apply":["K2Plus"],
        "content": cxTr('FR2835'),
        "supportVersion": 830,
        "isCfs":true
    },
    2836:{
        "code":"FR2836",
        "level": 1,
        "apply":["K2Plus"],
        "content": cxTr('FR2836'),
        "supportVersion": 830,
        "isCfs":true
    },
    2839:{
        "code":"FR2839",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FR2839"),
        "supportVersion": 830,
        "isCfs":true
    },
    2848:{
        "code":"FR2848",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FR2848'),
        "supportVersion": 830,
        "isCfs":true
    },
    2849:{
        "code":"FR2849",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FR2849'),
        "supportVersion": 830,
        "isCfs":true
    },
    2850:{
        "code":"FR2850",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FR2850'),
        "supportVersion": 830,
        "isCfs":true
    },
    2851:{
        "code":"FR2851",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FR2851'),
        "supportVersion": 830,
        "isCfs":true
    },

    2831:{
        "code":"FS2831",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr('FS2831'),
        "supportVersion": 830,
        "isCfs":true
    },
    2834:{
        "code":"FS2834",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FS2834"),
        "supportVersion": 830,
        "isCfs":true
    },
    2840:{
        "code":"FS2840",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FS2840"),
        "supportVersion": 830,
        "isCfs":true
    },
    2843:{
        "code":"FS2843",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FS2843"),
        "supportVersion": 830,
        "isCfs":true
    },
    2841:{
        "code":"TC2841",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("TC2841"),
        "supportVersion": 830
    },
    2111:{
        "code":"TE2111",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("TE2111"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-series-general-documents/2111-error-trouble-shooting"
    },
    2509:{
        "code":"TE2509",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("TE2509"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-series-general-documents/2509-error-trouble-shooting"
    },
    2515:{
        "code":"TE2515",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("TE2515"),
        "supportVersion": 830
    },
    2564:{
        "code":"TE2564",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("TE2564"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-series-general-documents/2564-error-trouble-shooting"
    },
    501:{
        "code":"TF0501",
        "level": 2,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("TF0501"),
        "supportVersion": 830,
    },
    116:{
        "code":"TR0116",
        "level": 3,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("TR0116"),
        "supportVersion": 830,
    },
    2571:{
        "code":"XD2571",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XD2571"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-series-general-documents/2121-error-trouble-shooting"
    },
    2000:{
        "code":"XS2000",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS2000"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-max/troubleshooting/2000-error-trouble-shooting"
    },
    2001:{
        "code":"XS2001",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS2001"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-max/troubleshooting/2001-error-trouble-shooting"
    },
    2060:{
        "code":"XS2060",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS2060"),
        "supportVersion": 830
    },
    2093:{
        "code":"XS2093",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS2093"),
        "supportVersion": 830
    },
    3000:{
        "code":"XS3000",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS3000"),
        "supportVersion": 830
    },
    3001:{
        "code":"XS3001",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS3001"),
        "supportVersion": 830
    },
    3002:{
        "code":"XS3002",
        "level": 1,
        "apply":["K1","K1C","K1MAX","K2Plus"],
        "content":cxTr("XS3002"),
        "supportVersion": 830,
        "wiki":"https://wiki.creality.com/zh/k1-flagship-series/k1-max/troubleshooting/3002-error-trouble-shooting"
    },
	2768:{
        "code":"CZ2768",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CZ2768"),
        "supportVersion": 830
    },
	2798:{
        "code":"CM2798",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CM2798"),
        "supportVersion": 830
    },
	2711:{
        "code":"CA2711",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("CA2711"),
        "supportVersion": 830
    },
	120:{
        "code":"CA0120",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("CA0120"),
        "supportVersion": 830
    },
	2710:{
        "code":"CA2710",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("CA2710"),
        "supportVersion": 830
    },
	2559:{
        "code":"CP2559",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("CP2559"),
        "supportVersion": 830
    },
	2852:{
        "code":"TR2852",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("TR2852"),
        "supportVersion": 830
    },
	2854:{
        "code":"TC2854",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("TC2854"),
        "supportVersion": 830
    },
	2856:{
        "code":"TC2856",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("TC2856"),
        "supportVersion": 830
    },
	2855:{
        "code":"TC2855",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("TC2855"),
        "supportVersion": 830
    },
	2762:{
        "code":"TE2762",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("TE2762"),
        "supportVersion": 830
    },
	2761:{
        "code":"TE2761",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("TE2761"),
        "supportVersion": 830
    },
	526:{
        "code":"TF0526",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("TF0526"),
        "supportVersion": 830
    },
	2853:{
        "code":"FH2853",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FH2853"),
        "supportVersion": 830
    },
	2857:{
        "code":"FM2857",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FM2857"),
        "supportVersion": 830
    },
	121:{
        "code":"FR0121",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FR0121"),
        "supportVersion": 830
    },
	122:{
        "code":"FR0122",
        "level": 1,
        "apply":["K2Plus"],
        "content":cxTr("FR0122"),
        "supportVersion": 830
    },
	2858:{
        "code":"FS2858",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FS2858"),
        "supportVersion": 830
    },
	2859:{
        "code":"FO2859",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FO2859"),
        "supportVersion": 830
    },
	2860:{
        "code":"FB2860",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FB2860"),
        "supportVersion": 830
    },
	2861:{
        "code":"FS2861",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FS2861"),
        "supportVersion": 830
    },
	2862:{
        "code":"FS2862",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FS2862"),
        "supportVersion": 830
    },
	528:{
        "code":"FO0528",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("FO0528"),
        "supportVersion": 830
    },
	511:{
        "code":"AC0511",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("AC0511"),
        "supportVersion": 830
    },
	512:{
        "code":"AC0512",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("AC0512"),
        "supportVersion": 830
    },
	513:{
        "code":"AC0513",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("AC0513"),
        "supportVersion": 830
    },
	514:{
        "code":"AC0514",
        "level": 2,
        "apply":["K2Plus"],
        "content":cxTr("AC0514"),
        "supportVersion": 830
    }
}

function updateErrorJsonTranslation() {
    for (let key in errorJson) {
        if(errorJson[key].supportVersion === 800){
            errorJson[key].content = qsTr(errorJson[key].code);
        }else{
            errorJson[key].content = cxTr(errorJson[key].code);
        }
    }
}
