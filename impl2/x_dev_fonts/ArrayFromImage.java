import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.InputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class ArrayFromImage {

	// Usage $0 image charw charh listchars
	public static void main(String[] args) throws IOException {
		BufferedImage img;
		try(InputStream is = Files.newInputStream(Paths.get(args[0]))) {
			img = ImageIO.read(is);
		}

		int charw = Integer.parseInt(args[1]);
		int charh = Integer.parseInt(args[2]);
		char[] list = args[3].toCharArray();

		int curc = 0;
		for(int y = 0; y < img.getHeight(); y += charh) {
			for(int x = 0; x < img.getWidth(); x += charw) {
				System.out.println("/* " + list[curc++] +
									" */");
				System.out.print("{");
				for(int col = 0; col < charw; col++) {
					long data = 0;
					long writeat = charh - 1;
					/* we write px msb to lsb */
					for(int row = 0; row < charh; row++) {
						int pxval = img.getRGB(x + col,
							y + row) & 0xffffff;
						if(pxval == 0xffffff) {
							/* ign */
						} else if(pxval == 0) {
							data |= (1 << writeat);
						} else {
							System.err.println(
								"unknown " +
								"color: " +
								pxval
							);
						}
						writeat--;
					}
					System.out.print(String.format("0x%x,",
									data));
				}
				System.out.println("}");
			}
		}
	}

}
