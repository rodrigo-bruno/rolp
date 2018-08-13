import java.lang.management.GarbageCollectorMXBean;
import java.lang.management.ManagementFactory;
import java.nio.ByteBuffer;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.ThreadLocalRandom;

public class NG3 {

    public static final int max_gens = 100;
    public static long allocated = 0;
    public static long allocated_ng = 0;
    public static final int buf_sz = 1024*1024;

    public static final LinkedList<HashSet<Object>> objects = new LinkedList<>();

    public static final Thread counter = new Thread(new Runnable() {
        @Override
        public void run() {
            long totalGarbageCollections = 0;

            while (true) {
                for(GarbageCollectorMXBean gc :
                        ManagementFactory.getGarbageCollectorMXBeans()) {

                    long count = gc.getCollectionCount();

				    if (count <= 0) {
				        continue;
					}

                    synchronized(counter) {
						if (count != totalGarbageCollections) {
						    System.out.println(String.format("%s: %d. %s: %d",
						        "Total Garbage Collections: ",
								 totalGarbageCollections,
								 ". Allocated Objects: ",
								 allocated));
						    allocated = 0;
						    allocated_ng = 0;
						}
                        while (count > totalGarbageCollections) {
                            objects.removeFirst();
                            totalGarbageCollections += 1;
                        }
                    }

                }

                try { Thread.sleep(1000); }
                catch (Exception e) {}
            }
        }
    });

	public static Object allocate() throws Exception {
			//if (allocated_ng == 5) { throw new Exception(); }
			//if (allocated_ng == 5) { throw new RuntimeException(); }
			//return new byte[buf_sz];
			return new Object();

			//return ByteBuffer.allocate(1).array();
	}

	public static Object allocate1() throws Exception {
			return allocate();
	}

    public static Object allocate2() throws Exception {
			return allocate();
	}

    public static Object iteration() throws Exception {
        Object object;
		HashSet hs;
	    synchronized(counter) {
		    while (objects.size() < 4) { // TODO - fix this!
                    objects.add(new HashSet<>());
            }
			if (allocated_ng < 10) {
				    allocated_ng += 1;
					// Add 1
             		object = allocate1();
					// Note: need to do this trick to ensure that the hashset
					// where the object is added is not already in the old gen.
					hs = new HashSet(objects.get(1 - 1));
                    hs.add(object);
                    objects.set(1 - 1, hs);

				    // Add 2
             		object = new byte[buf_sz];
					hs = new HashSet(objects.get(2 - 1));
                    hs.add(object);
                    objects.set(2 - 1, hs);

					// Add 3
					object = new byte[buf_sz];
					hs = new HashSet(objects.get(3 - 1));
                    hs.add(object);
                    objects.set(3 - 1, hs);

					// Add 4
             		object = allocate2();
					hs = new HashSet(objects.get(4 - 1));
                    hs.add(object);
                    objects.set(4 - 1, hs);
			}  else {
                    object = new byte[buf_sz];
            }
            allocated += 1;
        }
        return object;
	}

    public static void main(String[] args) {
        counter.start();

        while(true) {
		    try { iteration(); }
			catch (Exception e) { System.out.println("Caught Exception!"); }
            try { Thread.sleep(5); } catch (Exception e) {}
        }
    }
}
