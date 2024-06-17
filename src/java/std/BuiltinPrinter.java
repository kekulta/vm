package java.builtin;

public class BuiltinPrinter {
    public static void println(String s) {
        nativePrintln(s);    
    }

    private static native void nativePrintln(String s);
}
