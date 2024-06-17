public class RegisterNativesHelloWorldJNI {

    public native void register();
    public native String sayHello();

    public static void main(String[] args) {
        RegisterNativesHelloWorldJNI helloWorldJNI = new RegisterNativesHelloWorldJNI();
        helloWorldJNI.register();
        helloWorldJNI.sayHello();
    }
}
