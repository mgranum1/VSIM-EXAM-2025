//#include "MainWindow.h"
#include "Editor/MainWindow.h"
#include "../../../Soundsystem/resourcemanager.h"
#include "../../../Scripting/luamanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ResourceManager resourceMgr;         //Create sound manager
    MainWindow w(&resourceMgr);          //Pass pointer to MainWindow
    w.move(200, 100);
    w.show();

    LuaManager lua;
    lua.runFile("Assets/Scripts/test.lua");

    w.start();
    return a.exec();
}
