* 환경변수 설정
- MOBILE_LITE_HOME : openML 경로 설정
    ex) $ setenv MOBILE_LITE_HOME /home/openML
- MOBILE_LITE_CONFIG : DB file 및 openml.cfg 경로 설정
    ex) $ setenv MOBILE_LITE_CONFIG $MOBILE_LITE_HOME/dbspace
- LD_LIBRARY_PATH 설정
    ex) $ setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:$MOBILE_LITE_HOME/lib

* config.mk
- openML의 compile option을 설정하는 file
- OPTION : 제공되는 define option을 추가 삭제
- ENABLE_YACC : make시 parser compile 포함 설정 
    1 : parser compile 포함
    0 : parser compile 미포함
- JNI : openML JNI library 생성 설정
    1 : JNI library compile 포함
    0 : JNI libaray compile 미포함
- HAVE_READLINE : isql tool을 위한 interactive prompt 기능 설정
    1 : interactive prompt 기능 포함
    0 : interactive prompt 기능 미포함
- COMPILE_MODE : compile mode 설정
    Default : mode32 (Makefile 참조)
- COMPILER : compiler 설정 
    Default : gcc (Makefile 참조)

* make
- openML compile시 config.mk의 설정을 바탕으로 Makefile을 수행
- Debug(default)/Release mode 인자로 구분
    $ make release
- COMPILE_MODE, COMPILER를 인자로 설정. 설정시 config.mk의 값이 변경됨
    $ make mode64
    $ make arm-gcc-android
    $ make
- lib 폴더에 libopenml.a와 libopenml.so가 생성됨
- bison, flex 사용 버전
    bison 2.5, flex 2.5.34 이상 사용 권고

* JNI
- compile
a. config.mk의 'JNI=0'을 'JNI=1'로 변경
b. 환경변수 JAVA_HOME에 설치된 java directory를 설정
    ex) $ setenv JAVA_HOME /usr/lib/jvm/java
c. make
- 사용법
a. openML의 include 폴더에 있는 JEDB.java를 작업 폴더하위에 com.openml로 복사
b. openML의 lib 폴더에 생성된 JNI library인 libjedb.so를 링크 하도록 코드 추가
c. example.java 작성 (코드 작성시 JEDB.java에 정의된 함수들을 이용)
