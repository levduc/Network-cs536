import java.io.*;
import java.net.*;
import java.lang.Object;

public class fileclient {
	public static void main(String[] args) {
        /*Check for client input*/
		if (args.length != 5) {
			System.out.println("Usage: <hostname> <portnumber> <secretkey> <filename> <configfile>\n");
			System.exit(1);
		}

		if (args[2].length() < 10) {
			System.out.println("Secret Key is too short");
			System.exit(1);
		}
		if (args[2].length() > 20) {
			System.out.println("Secret Key is too long");
			System.exit(1);
		}
		/*Creating new socket*/
		try {
			Socket clientSocket = new Socket(args[0], Integer.parseInt(args[1]));
			/*Secret Key*/
			String secretKey = args[2];
			/*File Name*/
			String fileName = args[3];
			/*configFile*/
			String configFileName = args[4];
			File file = new File(configFileName);
			BufferedReader reader = null;
			int readSize = 0;
			try {
				reader = new BufferedReader(new FileReader(file));
				String temp = null;
				if ((temp = reader.readLine()) != null) {
					readSize = Integer.parseInt(temp);
				}
			} catch (Exception ex)
			{
				System.out.println("Exception occured when openning file");
			}
			String clientRequest = "$" + secretKey + "$" + fileName;
			DataOutputStream os = new DataOutputStream(clientSocket.getOutputStream());
			/*send request to server*/
			os.writeBytes(clientRequest);
			
			System.out.println(clientRequest);
			boolean end = false;
			try 
			{
				FileOutputStream fos = new FileOutputStream(fileName);
				byte[] contents = new byte[readSize];
        		BufferedOutputStream bos = new BufferedOutputStream(fos);
				InputStream is = clientSocket.getInputStream();
		        //No of bytes read in one read() call
		        int bytesRead = 0; 
		        while((bytesRead=is.read(contents))!=-1)
		            bos.write(contents, 0, bytesRead); 
				System.out.println("File Saved!");
			}
			catch (Exception e)
			{
			    e.printStackTrace();
			}
			clientSocket.close();
		} catch(Exception e)
		{
			System.out.println(e.toString());
		}
	}
}
