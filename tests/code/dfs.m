
       	                  
                                 	             init -o axioms�   discover(J), first-time() -o father(J), check-if-only-neighbor(J), 
			setcolor(255, 255, 0), {Q, Flag | !edge(Q), flag(Q, Flag), unvisited(Q), 
			Q != J | unvisited(Q), visited(host-id), flag(Q, 1)}.)   discover(J), !edge(J) -o return(host-id).8   check-if-only-neighbor(J), unvisited(X) -o unvisited(X).T   check-if-only-neighbor(Node), !edge(Node) -o return(host-id), setcolor(0, 255, 255).7   return(Q), unvisited(K), !edge(K) -o discover(host-id).z   return(Q), father(Father), !edge(Father), Father != host-id -o 
			father(Father), return(host-id), setcolor(0, 255, 255).A   return(Q), father(A), A = host-id -o stop(), 
			father(host-id).3   visited(K), unvisited(K), !edge(K) -o ack(host-id).[   ack(J), flag(J, MV104), flag(J2, MV179), J != J2, 
			MV179 = 1 -o flag(J, 0), flag(J2, 1).6   ack(J), flag(J, MV105) -o flag(J, 0), return(host-id).           ��                                     _init                                                                                               set-priority                                                                                        setcolor                                                            	                              setedgelabel                                                        	                               write-string                                                                                        add-priority                                                    k                                  edge                                                            )                                  flag                                                            �                                   unvisited                                                       �                                   father                                                          �                                   discover                                                        =                                   visited                                                         �                                   return                                                          s                                   check-if-only-neighbor                                                                               stop                                                            �                                   ack                                                             �                                     first-time                                                       
�  
   r     �     �  �  �  I  �  +  @ 0       @ 0       @ 0     0       @ 0        @ 0        @ 0      0       �  @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       >  @ 0        @ 0        @ 0      0       @ 0       @ 0       @ 0     0       �  @ 0        @ 0        @ 0      0       @ 0       @ 0       @ 0     0       \  @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       �  @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0         @ 0       @ 0       @ 0     0       @ 0	       @ 0	       @ 0	     0       �  @ 0       @ 0       @ 0     0       @ 0	       @ 0	       @ 0	     0       ,  @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       @ 0       @ 0       @ 0     0       �   @ 0	       @ 0	       @ 0	     0       @ 0       @ 0       @ 0     0       @
 0          @   �        0 �
 $    B  0!@0 0#  ��   0 � E    B  0!@0 0#  @0     0�   0�   ��   0 � 4    �0!� &    B  0"@
0 0$  ���   0 �!  `h   � b    �0!�	 T    B  0"@	0   @0 0$  @0     0�   0�   ���   0 � 6    B  0!� &    B  0"@0 0$  ��� 	   0 � p    B  0!� `    �0"�#   `I   �#   `8   @0   0    @0  0   ���0 �!    `e   � _    �0!�"   `H   � B    B 0"@0  0    @0   0   ���
   0 � 5    B  0!@0   0    @0 ��    0 �     �0!@0   ��   0 � 2    �0!�  $    B  0"@
0 0$  ��   0 � 4    B  0!�  $    B  0"@0 0$  ��    0 �!  `f   � `    �0!�  R    B  0"@	0   @0 0$  @0     0�   0�   ��   0 �!  `)   � #    �0!@@	0 ��    0 � �    �0!@	0   @0   @0�    0�   0    {   �  u    �0"�#   `^   � X    B 0#� H    B 0$@0  @0 0& @0  0   �����   0 �  "    B  0!@0 0#  �    0 � 4    B  0!�  $    B  0"@0 0$  ��    0 � 2    �0!�  $    B 0"@
0 0$ ��   0 �	 m    �0!�" `X   �  R    B 0"@	0  @0 0$ @0     0�   0�   ��   0 �	 0    �0!�" `   @@	0 ��    0 �     �0!@0  ��   0 �  C    B  0!@0 0#  @0     0�   0�   �  	   0 � p    B  0!� `    �0"�#   `I   �#   `8   @0   0    @0  0   ���
   0 � 5    B  0!@0   0    @0 ��    �
 �    �0 @	0   @0   @0�    0�   0    {   �  u    �0!�"   `^   � X    B 0"� H    B 0#@0  @0 0% @0  0   ���� �    ~	  �  }	   �0 �    `    @ 0	       �    `    @ 0	       �    `    @ 0	     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       � 	   `    @ 0       � 	   `    @ 0       � 	   `    @ 0     0       � 	   `    @ 0       � 	   `    @ 0       � 	   `    @ 0     0       � 	   `    @ 0       � 	   `    @ 0       � 	   `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0	       �    `    @ 0	       �    `    @ 0	     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0	       �    `    @ 0	       �    `    @ 0	     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0        �    `    @ 0        �    `    @ 0      0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0        �    `    @ 0        �    `    @ 0      0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �     `    @ 0       �     `    @ 0       �     `    @ 0     0       �     `    @ 0       �     `    @ 0       �     `    @ 0     0       �     `    @ 0       �     `    @ 0       �     `    @ 0     0       �    `    @ 0       �    `    @ 0       �    `    @ 0     0       �    `    @ 0        �    `    @ 0        �    `    @ 0      0       �    `    @
 0       @   � �     �   � �    �0 �
 �    �0!@	0  @0  @0�    0�   0    {   �  u    �0"�#  `^   � X    B 0#� H    B 0$@0  @0 0& @0  0   ����� �    
2   �
 1    �0 �  #    B  0!@0 0#  � �    
/   � .    �0 �      �0!@0  �� �    S   � R    �0 �  D    B  0!@0 0#  @0     0�   0�   � �    B   � A    �0 � 3    �0!�  %    B 0"@
0 0$ �� �    }   � |    �0 �	 n    �0!�" `Y   �  S    B 0"@	0  @0 0$ @0     0�   0�   �� �    	@   � ?    �0 �	 1    �0!�" `   @@	0 �� �    	D   � C    �0 � 5    B  0!�  %    B  0"@0 0$  �� �    �   �     �0 � q    B  0!� a    �0"�#   `J   �#   `9   @0   0    @0  0   ��� �    E   � D    �0 � 6    B  0!@0   0    @0 �� �    