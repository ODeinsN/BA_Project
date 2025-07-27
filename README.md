# BA_Project
This repository consists of the practical implemenation of my bachelor thesis, which focused on generating dialogs using locally run LLMs while fetching domain specific information, a conversation partner should have, 
by generating cypher queries based on user messages and enriching retrieved information into a generated answer.

![llm-inference-chart](https://github.com/user-attachments/assets/e2b62c4d-3933-4e91-8a38-98542bc6a4ed)

To integrate LLMs locally, a modified version of the [Llama-Unreal plugin](https://github.com/getnamo/Llama-Unreal) by getnamo has been used, which makes it possible to use multiple GPUs, for running the LLMs.

The knowledge of a character is defined via InformationCollections and InformationBundles from the [InfodemicDB Plugin](https://github.com/tortle-sh/InfodemicSystemPlugin/tree/master), which stores information as data assets and automatically fills 
a Neo4j database with the relevant information.

![messageToQueryToMessage (1)](https://github.com/user-attachments/assets/cedf23b4-bfab-4d81-bd40-8ac118f460e7)

## Setup
To run this demo, you need to run Neo4j in a docker container and download ai models, for example from huggingface.

### Neo4j
1. Enable Hardware virtualisation in your BIOS
2. Setup Docker on your PC
3. Setup a Neo4j container using the docker-compose.yaml
4. Go to DefaultEngine.ini and adjust following parameters to match your db config:
```ini
[InfodemicDbConfig]
Url="http://localhost:7474/db/neo4j/tx/commit"
Username=neo4j
Password=password
DbSubsystemClassPath=/Script/InfodemicNeo4jConnector.InfodemicDb_Neo4j_Subsystem
```

### LLM
This plugin makes use of locally run LLM models, which are not included in this project, meaning you have to download them yourselfs.
You can use different Models for the generations of dialogs and db queries or you could just use the same for both.

1. Download a model(s) from Huggingface in the gguf format
2. Add them to the `Project/Saved/Models/` directory
3. Open this projcet via a IDE like Visual Studio or Rider and and run it
4. Open the `Content > Chat > ChatTester > BP_ChatTestActor`
5. Select the DialogModel/CypherModel Component
6. In the details tab, go to `LLM Model Component > Model Params > Path to Model` and enter the path to your model (see the [Unreal-Llama Plugin documentation](https://github.com/getnamo/Llama-Unreal?tab=readme-ov-file#how-to-use---basics))
   - note: if the path is prefixed with `./` then it is assumed that it is located in `Project/Saved/Models/`

### LLM - Multi GPU setup
By default both models will be loaded at the same time and run in parallel on your main GPU. If you have access to multiple GPUs,
you can configure the `LLM Model Component > Model Params > Tensor Split` parameter to either split the models between multiple GPUs or
run assign the models to run on different GPUs, e.g. the dialog model on GPU0 and cypher model on GPU1.

The TensorSplit Parameter is an array of integer, which define the distribution of the model layers across the your GPUs.
If you have two GPUs and you want to run you model on 70% on GPU0 and 30% on GPU1, make the array two elements long and set index[0] to 7 and index[1] to 3

<img width="340" height="92" alt="image" src="https://github.com/user-attachments/assets/9f736114-c633-4681-b5e5-872f13ad0c37" />

If you want one model to run only on GPU0 and the other on gpu1 you can set a (1,0) and (0,1) ratio.

<img width="319" height="82" alt="image" src="https://github.com/user-attachments/assets/b70e1dee-350b-4fcc-9c98-6e0e65b98f18" />

> Note: Currently it is not obvious which GPU is assigned to which index. Sometimes other devices or programs count as display devices which might require it to add more elements to the array.
> The current best way is to open task manager, experiment with values and see when the GPU load goes up








