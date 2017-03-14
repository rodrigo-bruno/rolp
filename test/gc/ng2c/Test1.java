/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author underscore
 */
public class Test1 {
    
    public static Boolean s = new Boolean(false);
    public static Integer i;
    
    static { i = new Integer(1); }
    

    public static void main(String[] args) {
        Object o = new Object();
        Object[] os = new Object[1024];
        int[][][] is = new int[1024][1024][0];
        System.out.println(o);
        System.out.println(os);
        System.out.println(is);
        System.out.println(s);
        System.out.println(i);
    }
}
