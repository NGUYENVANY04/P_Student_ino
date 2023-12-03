// firebase_handler.h

#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <FirebaseESP32.h>

class FirebaseHandler
{
public:
    FirebaseHandler();
    void connectWiFi();
    void initializeFirebase();
    void updateFirebase(String path, String time, String day);

private:
    FirebaseData fbdo;
};

#endif
