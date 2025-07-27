# BA_Project
This repository consists of the practical implemenation of my bachelor thesis, which focused on generating dialogs using locally run LLMs while fetching domain specific information, a conversation partner should have, 
by generating cypher queries based on user messages and enriching retrieved information into a generated answer.

![llm-inference-chart](https://github.com/user-attachments/assets/e2b62c4d-3933-4e91-8a38-98542bc6a4ed)

To integrate LLMs locally, a modified version of the [Llama-Unreal plugin](https://github.com/getnamo/Llama-Unreal) by getnamo has been used, which makes it possible to use multiple GPUs, for running the LLMs.

The knowledge of a character is defined via InformationCollections and InformationBundles from the [InfodemicDB Plugin](https://github.com/tortle-sh/InfodemicSystemPlugin/tree/master), which stores information as data assets and automatically fills 
a Neo4j database with the relevant information.

![messageToQueryToMessage (1)](https://github.com/user-attachments/assets/cedf23b4-bfab-4d81-bd40-8ac118f460e7)
