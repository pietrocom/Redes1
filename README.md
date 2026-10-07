Este é o trabalho de redes 1 da UFPR 2026/2. 

O objetivo é estabelecer uma comunicação entre um cliente e um servidor com o intúito de transferir arquivos.
O protocolo deve ser implementado utilizando raw_sockets sobre mensagens ethernet.

O cliente pode rodar os comandos:
 - get (recebe o arquivo especificado)
 - put (envia o arquivo especificado)
 - ls  (lista os arquivos do diretório atual do servidor)
 - cd  (muda para o diretório especificado do servidor)
 - lls (ls local)
 - lcd (cd local)

São características do protocolo:
 - para-e-espera
 - paridade vertical de 32 bits par
 - menor mensagem tem 64 bytes
 - maior mensagem tem 128 bytes

Formato das mensagens:
| Preâmbulo (7B) | Marcador Início (1B) | End Destino (6B) | End Origem (6B) | Tamanho (2B) | Sequência (4b) | Tipo (4b) | Dados (45 a 117B) | CRC (4B) |
onde B é bytes e b é bits. 

Sobre os tipos:
 - 0: ACK
 - 1: NACK
 - 2: ERRO
 - 3: cd
 - 4: ls
 - 5: get
 - 6: put
 - 7: (a definir)
 - 8: OK
 - 9: fim da transmissão
 - 10:
 - 11:
 - 12:
 - 13: Dados do arquivo
 - 14: Tamanho do arquivo
 - 15: Mostrar na tela

A implementação de um timeout é obrigatória. Note que não há timeout para os tipos 0,1 e 2.
 - 
