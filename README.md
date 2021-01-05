# EE450 Final Project - UNIX Socket Programming
Instructor: [Prof. Ali Zahid](https://viterbi.usc.edu/directory/faculty/Zahid/Ali), Fall 2020, University of Southern California.

## Overview of Socket Project

In this project, I implemented a customized social recommendation system based on STL libraries and TCP/UDP protocols, which contains four major components:
* Client 1 and 2
* One Main Server
* Back-end Server A and B 
* Facebook Social Network Database

### Simple Demo

<p align="center"><img src="img/Demo.gif" alt="Demo" width="500" /></p>

---

# Client/Server Architecture

There is a total of 5 communication end-points:
* **Client 1 & 2:** Represented two different users, possibly in different countries.
* **Main Server:** Interacted with the clients and the backend servers.
* **Backend Server A & B:** Generated the new friend based on the query.
* **User Data:** Stored as data1.txt and data2.txt in Back-end servers' local file system.

<p align="center"><img src="img/Data_Structure.png" alt="Data_Structure" width="500" /></p>