// program hello world
byte prog1[] = {STRING,'h','e','l','l','o',' ','w','o','r','l','d','\n',0,
                PRINT,
                STOP};
//byte prog1[] = {STRING,'t','e','s','t',0,SET,'s',
//                CHAR,'a',SET,'c',
//                STRING,'p','a','s','s','e','d',0,SET,'s',
//                GET,'c',INCREMENT,SET,'c',
//                GET,'c',PRINTLN,
//                STOP};
//byte prog1[] = {STRING,'p','r','o','g','3',0,
//                FORK,WAITUNTILDONE,STOP};
////                
// program test_vars
byte prog2[] = {STRING,'t','e','s','t',0,SET,'s',
                CHAR,'a',SET,'c',
                INT,1,7,SET,'i', // 263
                FLOAT,66,246,230,102,SET,'f', //123.45
                STRING,'p','a','s','s','e','d',0,SET,'s',
                GET,'c',INCREMENT,SET,'c',
                GET,'i',DECREMENT,SET,'i',
                GET,'f',INCREMENT,SET,'f',
                GET,'s',PRINTLN,
                GET,'c',PRINTLN,
                GET,'i',PRINTLN,
                GET,'f',PRINTLN,
                STOP};

// program test_loop
byte prog3[] = {INT,0,0,SET,'i',
                LOOP,
                GET,'i',INCREMENT,SET,'i',
                GET,'i',PRINTLN,
                MILLIS,INT,3,232,PLUS, // 1000
                DELAYUNTIL,
                ENDLOOP};

// program test_if
byte prog4[] = {INT,0,3,SET,'a',
                CHAR,5,SET,'b',
                GET,'a',GET,'b',EQUALS,
                IF,7,
                STRING,'T','r','u','e',0,
                PRINTLN,
                ELSE,8,
                STRING,'F','a','l','s','e',0,
                PRINTLN,
                ENDIF,
                CHAR,3,SET,'b',
                GET,'a',GET,'b',EQUALS,
                IF,7,
                STRING,'T','r','u','e',0,
                PRINTLN,
                ELSE,8,
                STRING,'F','a','l','s','e',0,
                PRINTLN,
                ENDIF,
                STOP};

// program test_while
byte prog5[] = {INT,0,0,SET,'i',
                GET,'i',CHAR,5,LESSTHAN,
                WHILE,5,14,
                GET,'i',INCREMENT,SET,'i',
                GET,'i',PRINTLN,
                MILLIS,INT,3,232,PLUS, // 1000
                DELAYUNTIL,
                ENDWHILE,
                STOP};

// program blink
byte prog6[] = {INT,0,LED_BUILTIN,SET,'p',
                GET,'p',INT,0,OUTPUT,PINMODE,
                LOOP,
                GET,'p',INT,0,HIGH,DIGITALWRITE,
                MILLIS,INT,1,244,PLUS, // 500
                DELAYUNTIL,
                GET,'p',INT,0,LOW,DIGITALWRITE,
                MILLIS,INT,1,244,PLUS, // 500
                DELAYUNTIL,
                ENDLOOP};

// program read_file
byte prog7[] = {CHAR,20,SET,'i',
                STRING,'t','e','s','t','_','v','a','r','s',0,
                INT,0,0,
                OPEN,
                GET,'i',
                WHILE,2,8,
                READCHAR,TOINT,PRINTLN,
                GET,'i',DECREMENT,SET,'i',
                ENDWHILE,
                STOP};

// program write_file
byte prog8[] = {STRING,'n','e','w','_','f','i','l','e',0,SET,'s',
                GET,'s',
                INT,0,20,
                OPEN,
                STRING,'t','e','x','t','\n',0,WRITE,
                CHAR,'a',WRITE,
                INT,98,99,WRITE, // 25187
                FLOAT,61,204,204,205,WRITE, // 0.1
                GET,'s',
                INT,0,20,
                OPEN,
                READSTRING,PRINTLN,
                READCHAR,PRINTLN,
                READINT,PRINTLN,
                READFLOAT,PRINTLN,
                STOP};

// program test_fork
byte prog9[] = {INT,0,0,SET,'i',
                LOOP,
                GET,'i',INCREMENT,SET,'i',
                GET,'i',PRINTLN,
                GET,'i',CHAR,5,EQUALS,
                IF,14,
                STRING,'t','e','s','t','_','w','h','i','l','e',0,
                FORK,WAITUNTILDONE,
                ENDIF,
                MILLIS,INT,3,232,PLUS, // 1000
                DELAYUNTIL,
                ENDLOOP};
