public class Test2 {
    public static int testField = 10;

    static {
        System.out.println("Static block");
    }

    {

        System.out.println("Block 1");
    }

    {

        System.out.println("Block 2");
    }

    public static void main(String[] args) {
        System.out.println("Hello, test!");
    }

    public String testFunc() {
        int testInt = 10000;
        System.out.println(11 + testInt + testField);

        return "test return";
    }

    public static String testStatic() {
        int testInt = 2;
        System.out.println(2 + testInt);

        return "test return";
    }
}
