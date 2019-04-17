public class Rank {
  public static void main(String[] args) {
    String[][] all;
    int best, rest;
    int nb = both[best].length;
    int nr = both[rest].length;
    
    String[] order = new String[0];
    for(int i = 0; i < all.length; i++) {
      if(independent(all[i])) {
        for(int j = 0; j < all[i].length; j++) {
          double b = look2(both[best], i, j, 0.001);
          double r = look2(both[rest], i, j, 0.001);
          b = b / (nb + 0.00001);
          r = r / (nr + 0.00001);
          
        }
      }
    }
  }
}