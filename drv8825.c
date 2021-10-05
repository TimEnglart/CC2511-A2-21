struct stepper_motor {
    void (*update)();
    char title[50];
};

void b() {

}
void a() {
    struct stepper_motor smx;
    if(smx.title == "s") {
        smx.update();
    }
}
