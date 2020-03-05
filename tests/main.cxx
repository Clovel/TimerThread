#include "TimerThread.hxx"

#include <iostream>

int main()
{
    TimerThread t;
    // Timer fires once, one second from now
    t.addTimer(1000*1000, 0,
             []() {
                 std::cout << "Non-periodic timer fired" << std::endl;
             });
    // Timer fires every second, starting five seconds from now
    t.addTimer(5000*1000, 1000*1000,
             []() {
                 std::cout << "Timer fired 0" << std::endl;
             });
    // Timer fires every second, starting now
    t.addTimer(0, 1000*1000,
             []() {
                 std::cout << "Timer fired 1" << std::endl;
             });
    // Timer fires every 100ms, starting now
    t.addTimer(0, 100*1000,
             []() {
                 std::cout << "Timer fired 2" << std::endl;
             });
    // Timer fires once, 10 seconds from now
    t.addTimer(10000*1000, 0,
             []() {
                 std::cout << "Nice work Clovel !" << std::endl;
             });

    for(;;){
        //
    }

    return EXIT_SUCCESS;
}