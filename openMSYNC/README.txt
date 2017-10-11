****************************************************************************
*** Linux 상에서의 build 방법
****************************************************************************
* 환경변수 설정
- openML build 환경 구축에서 설정하는 환경 변수를 사용. 
- 주의점: openMSYNC 빌드 이전에 openML을 빌드해 두어야 한다.

* config.mk
- openMSYNC의 compile option을 설정하는 file
- OPTION : 제공되는 define option을 추가 삭제
- JNI : openMSYNC JNI library 생성 설정
    1 : JNI library compile 포함
    0 : JNI libaray compile 미포함
- COMPILE_MODE : compile mode 설정
    Default : mode64 (Makefile 참조)
- COMPILER : compiler 설정 
    Default : gcc (Makefile 참조)

* make
- openMSYNC compile시 config.mk의 설정을 바탕으로 Makefile을 수행
- Debug(default)/Release mode 인자로 구분
    $ make release
- COMPILE_MODE, COMPILER를 인자로 설정. 설정시 config.mk의 값이 변경됨
    $ make mode64
    $ make arm-gcc-android
    $ make
- lib 폴더에 libmSyncClient.a와 libmSyncClient.so가 생성됨
  
* JNI
- compile
a. config.mk의 'JNI=0'을 'JNI=1'로 변경
b. 환경변수 JAVA_HOME에 설치된 java directory를 설정
    ex) $ setenv JAVA_HOME /usr/lib/jvm/java
c. make
- lib 폴더에 libmSyncClientJNI.so가 생성됨


- 사용법
a. openMSYNC/client/mSyncClientJNI/com 폴더를 작업 폴더하위에 복사
b. openMSYNC의 lib 폴더에 생성된 JNI library인 libmSyncClientJNI.so를 링크 하도록 코드 추가
c. example.java 작성 (코드 작성시 mSyncClientJNI.java에 정의된 함수들을 이용)
d. openML과 연동할 경우 
  d-1. openML의 include 폴더에 있는 JEDB.java를 작업 폴더하위에 com/openml로 복사
  d-2. openML의 lib 폴더에 생성된 JNI library인 libjedb.so를 링크 하도록 코드 추가
  d-3. example.java 작성 (코드 작성시 JEDB.java에 정의된 함수들을 이용)

****************************************************************************
*** Windows 상에서의 build 방법
****************************************************************************
* 컴파일러는 VC 6.0을 기준으로 되어 있음.
* Workspace 로딩 및 컴파일 순서
   (주의점: openMSYNC 빌드 이전에 openML을 빌드해 두어야 한다.)
a. openMSYNC/openMSYNC.dsw 를 loading
b. "Project setting"에서 mSyncClientJNI 프로젝트를 선택하고  "C/C++" 탭의 Category를 "Preprocessor"로
   전환한 후 include directories에서 JAVA JDK의 include 경로를 포함해 준다.
   
   현재 기본적으로 다음과 같은 경로를 포함하도록 하고 있으니 적절하게 수정해서 사용한다.
   C:\Program Files\Java\jdk1.7.0_05\include,C:\Program Files\Java\jdk1.7.0_05\include\win32
c. Admin -> mSyncDBCTL -> mSync -> mSyncClient -> mSyncClientJNI 순서로 빌드 하도록 한다.  


