#ifndef ControlKey_h__
#define ControlKey_h__

#define CON_BIT( bit )  BIT( bit )

enum  ControlKey
{
	//Control Key
	CT_MOVE_LEFT    ,
	CT_MOVE_RIGHT   ,
	CT_MOVE_FORWARD ,
	CT_MOVE_BACK    ,
	CT_TURN_LEFT    ,
	CT_TURN_RIGHT   ,
	CT_USE          ,
	CT_ATTACK       ,
	CT_JUMP         ,
	CT_DEFENSE      ,

	CT_CONTROL_KEY ,
	CT_TURN_BIT = CON_BIT( CT_TURN_LEFT ) | 
				  CON_BIT( CT_TURN_RIGHT ) ,
	CT_MOVE_BIT = CON_BIT( CT_MOVE_LEFT ) | 
				  CON_BIT( CT_MOVE_RIGHT ) | 
				  CON_BIT( CT_MOVE_FORWARD )| 
			      CON_BIT( CT_MOVE_BACK ),
	//Hot Key
	CT_HOT_KEY = CT_CONTROL_KEY + 1,

	CT_FAST_PLAY_1 ,
	CT_FAST_PLAY_2 ,
	CT_FAST_PLAY_3 ,
	CT_FAST_PLAY_4 ,
	CT_FAST_PLAY_5 ,
	CT_FAST_PLAY_6 ,
	CT_FAST_PLAY_7 ,
	CT_FAST_PLAY_8 ,
	CT_FAST_PLAY_9 ,
	CT_FAST_PLAY_0 ,

	CT_SHOW_EQUIP_PANEL ,
	CT_SHOW_ITEM_BAG_PANEL ,
	CT_SHOW_SKILL_PANEL ,
};


#define KEY_LBUTTON             0x01
#define KEY_RBUTTON             0x02
#define KEY_CANCEL              0x03
#define KEY_MBUTTON             0x04
#define KEY_BACK                0x08
#define KEY_TAB                 0x09
#define KEY_CLEAR               0x0C
#define KEY_RETURN              0x0D
#define KEY_SHIFT               0x10
#define KEY_CTRL                0x11
#define KEY_ALT                 0x12
#define KEY_PAUSE               0x13
#define KEY_CAPITAL             0x14
#define KEY_ESCAPE              0x1B
#define KEY_ESC                 0x1B
#define KEY_SPACE               0x20
#define KEY_PRIOR               0x21
#define KEY_NEXT                0x22
#define KEY_END                 0x23
#define KEY_HOME                0x24
#define KEY_LEFT                0x25
#define KEY_UP                  0x26
#define KEY_RIGHT               0x27
#define KEY_DOWN                0x28
#define KEY_SELECT              0x29
#define KEY_PRINT               0x2A
#define KEY_EXECUTE             0x2B
#define KEY_SNAPSHOT            0x2C
#define KEY_INSERT              0x2D
#define KEY_DELETE              0x2E
#define KEY_HELP                0x2F
#define KEY_0                   0x30
#define KEY_1                   0x31
#define KEY_2                   0x32
#define KEY_3                   0x33
#define KEY_4                   0x34
#define KEY_5                   0x35
#define KEY_6                   0x36
#define KEY_7                   0x37
#define KEY_8                   0x38
#define KEY_9                   0x39
#define KEY_A                   0x41
#define KEY_B                   0x42
#define KEY_C                   0x43
#define KEY_D                   0x44
#define KEY_E                   0x45
#define KEY_F                   0x46
#define KEY_G                   0x47
#define KEY_H                   0x48
#define KEY_I                   0x49
#define KEY_J                   0x4A
#define KEY_K                   0x4B
#define KEY_L                   0x4C
#define KEY_M                   0x4D
#define KEY_N                   0x4E
#define KEY_O                   0x4F
#define KEY_P                   0x50
#define KEY_Q                   0x51
#define KEY_R                   0x52
#define KEY_S                   0x53
#define KEY_T                   0x54
#define KEY_U                   0x55
#define KEY_V                   0x56
#define KEY_W                   0x57
#define KEY_X                   0x58
#define KEY_Y                   0x59
#define KEY_Z                   0x5A
#define KEY_LWIN                0x5B
#define KEY_RWIN                0x5C
#define KEY_APPS                0x5D
#define KEY_NUMPAD0             0x60
#define KEY_NUMPAD1             0x61
#define KEY_NUMPAD2             0x62
#define KEY_NUMPAD3             0x63
#define KEY_NUMPAD4             0x64
#define KEY_NUMPAD5             0x65
#define KEY_NUMPAD6             0x66
#define KEY_NUMPAD7             0x67
#define KEY_NUMPAD8             0x68
#define KEY_NUMPAD9             0x69
#define KEY_MULTIPLY            0x6A
#define KEY_ADD                 0x6B
#define KEY_SEPARATOR           0x6C
#define KEY_SUBTRACT            0x6D
#define KEY_DECIMAL             0x6E
#define KEY_DIVIDE              0x6F
#define KEY_F1                  0x70
#define KEY_F2                  0x71
#define KEY_F3                  0x72
#define KEY_F4                  0x73
#define KEY_F5                  0x74
#define KEY_F6                  0x75
#define KEY_F7                  0x76
#define KEY_F8                  0x77
#define KEY_F9                  0x78
#define KEY_F10                 0x79
#define KEY_F11                 0x7A
#define KEY_F12                 0x7B
#define KEY_F13                 0x7C
#define KEY_F14                 0x7D
#define KEY_F15                 0x7E
#define KEY_F16                 0x7F
#define KEY_F17                 0x80
#define KEY_F18                 0x81
#define KEY_F19                 0x82
#define KEY_F20                 0x83
#define KEY_F21                 0x84
#define KEY_F22                 0x85
#define KEY_F23                 0x86
#define KEY_F24                 0x87
#define KEY_NUMLOCK             0x90
#define KEY_SCROLL              0x91
#define KEY_LSHIFT              0xA0
#define KEY_RSHIFT              0xA1
#define KEY_LCTRL               0xA2
#define KEY_RCTRL               0xA3
#define KEY_LALT                0xA4
#define KEY_RALT                0xA5
#define KEY_PROCESSKEY          0xE5
#define KEY_ATTN                0xF6
#define KEY_CRSEL               0xF7
#define KEY_EXSEL               0xF8
#define KEY_EREOF               0xF9
#define KEY_PLAY                0xFA
#define KEY_ZOOM                0xFB
#define KEY_NONAME              0xFC
#define KEY_PA1                 0xFD
#define KEY_OEM_CLEAR           0xFE


#endif // ControlKey_h__