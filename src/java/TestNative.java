import java.io.InputStream;

public class TestNative {
    public static void main(String args) {
        System.out.println("Hello, JNI!");
    }

    public static native void jniMethod();

    public static native void staticJniMethod();

    private static native void setIn0(
            InputStream in0,
            InputStream in1,
            InputStream in2,
            InputStream in3,
            InputStream in4,
            InputStream in5,
            InputStream in6,
            InputStream in7,
            InputStream in8,
            InputStream in9,
            InputStream in10,
            InputStream in11,
            InputStream in12,
            InputStream in13,
            InputStream in14,
            InputStream in15,
            InputStream in16,
            InputStream in17);
}
