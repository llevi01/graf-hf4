# CLion minta Grafikaházihoz. - Clion template for Computer Grafics
## Linux és Windows-ra egyaránt használható.  - For Linux and Windows

#### Windows:
Egy lépést kell egyedül elvégezni, hogy a /bin mappában lévő
*.dll fájlokat a létrehozott *cmake-build-release* és *cmake-build-debug*
mappába kell másolni.
#### Linux:
Telepíteni kell a glew és glut könyvtárakat.
**libglew-dev** és **freeglut3-dev**  
*Debian parancs:* sudo apt-get install libglew-dev freeglut3-dev 

### English:
#### Windows:
Only one step needs to be done, copy the *.dll files from /bin to
*cmake-build-release* and *cmake-build-debug*, witch were created by Clion.
#### Linux:
**libglew-dev** and **freeglut3-dev** needs to be installed.  
*Debian command:* sudo apt-get install libglew-dev freeglut3-dev 
