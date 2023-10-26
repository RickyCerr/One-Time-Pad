# One-Time-Pad
One time pad client-server encryption and decryption. Operating systems.

Must provide or create a key with "keygen.c". Can use "plaintext[1-5]" as the input text, or you can create your own. 
You must name an output file while running the program. Calling make will create an encrypt and decrypt for both client and server. 
You must run the server before the client. Encryption vs decryption cannot be on the same port. 

Example 
./enc_server 50864 & <---- encryption server running in the background ( & ) at port 50864
