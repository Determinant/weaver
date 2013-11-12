# 
# ===============================================================
#    Description:  Line reachability program in Python.
# 
#        Created:  11/11/2013 03:17:55 PM
# 
#         Author:  Ayush Dubey, dubey@cs.cornell.edu
# 
# Copyright (C) 2013, Cornell University, see the LICENSE file
#                     for licensing agreement
# ===============================================================
# 

import sys
sys.path.append('../bindings/python')

import client

# creating line graph
nodes = []
num_nodes = 100
coord_id = 0
c = client.Client(client._CLIENT_ID+1, coord_id)

tx_id = c.begin_tx()
for i in range(num_nodes):
    nodes.append(c.create_node(tx_id))
    print 'Created node ' + str(i)
c.end_tx(tx_id)
tx_id = c.begin_tx()
for i in range(num_nodes-1):
    c.create_edge(tx_id, nodes[i], nodes[i+1])
    print 'Created edge ' + str(i)
c.end_tx(tx_id)
print 'Created graph'

rp = client.ReachParams(dest=nodes[num_nodes-1])
print 'Created reach param: mode = ' + str(rp.mode) + ', reachable = ' + str(rp.reachable)
for i in range(num_nodes):
    prog_args = [(nodes[i], rp)]
    response = c.run_reach_program(prog_args)
    print 'From node ' + str(i) + ' to node ' + str(num_nodes-1) + ', reachable = ' + str(response.reachable)