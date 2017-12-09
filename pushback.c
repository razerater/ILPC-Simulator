/*
 * Check if various stages of our pipeline require stalls, forwarding, etc.
 * Then push the contents of our various pipeline stages through the pipeline.
 */
void iplc_sim_push_pipeline_stage()
{
    int i;
    int data_hit=1;

    /* 1. Count WRITEBACK stage is "retired" -- This I'm giving you */
    if (pipeline[WRITEBACK].instruction_address) {
        instruction_count++;
        if (debug)
            printf("DEBUG: Retired Instruction at 0x%x, Type %d, at Time %u \n",
                   pipeline[WRITEBACK].instruction_address, pipeline[WRITEBACK].itype, pipeline_cycles);
    }

    /* 2. Check for BRANCH and correct/incorrect Branch Prediction */  //sam
    if (pipeline[DECODE].itype == BRANCH) {
        int branch_taken = pipeline[FETCH].instruction_address;
        if (branch_taken == pipeline[DECODE].instruction_address + 4) {
            //branch not taken
            if (branch_predict_taken == 0) {
                //branch prediction correct
                correct_branch_predictions++;
            }
            else {
                //branch prediction incorrect - stall one cycle
                branch_prediction_incorrect();
            }
        }
        else {
            //branch taken
            if (branch_predict_taken == 1) {
                //branch prediction correct
                correct_branch_predictions++;
            }
            else {
                //branch prediction incorrect - stall one cycle
                branch_prediction_incorrect();
            }
        }
    }

    /* 3. Check for LW delays due to use in ALU stage and if data hit/miss
     *    add delay cycles if needed.
     */
    if (pipeline[MEM].itype == LW) {
        //int inserted_nop = 0; Do we need this?
        cache_access++;
        data_hit = iplc_sim_trap_address(pipeline[MEM].stage.lw.data_address);
        unsigned int canForward = 0;
        if(!data_hit) {
            // if MEM.dest_reg is equal to reg1 or reg2 in either DECODE or ALU -> then don't stall and add address to cache
            // else miss

            //Check the decode and mem for an rtype. Unnecessary line, but here for formatting.
            if(pipeline[DECODE].itype == RTYPE || pipeline[ALU].itype == RTYPE){
              //Check the registers of the rtype if we have an rtype in decode
              if(pipeline[DECODE].itype == RTYPE){
                if(pipeline[MEM].stage.lw.dest_reg == pipeline[DECODE].stage.rtype.reg1 ||
                   pipeline[MEM].stage.lw.dest_reg == pipeline[DECODE].stage.rtype.reg2) {
                   canForward = 1;
                }
              }
              if(pipeline[ALU].itype == RTYPE){
                //Check the registers of the rtype if we have an rtype in alu
                if(pipeline[MEM].stage.lw.dest_reg == pipeline[ALU].stage.rtype.reg1 ||
                   pipeline[MEM].stage.lw.dest_reg == pipeline[ALU].stage.rtype.reg2) {
                    canForward = 1;
                }
              }
            }
            //Check the decode and mem for a branch. Unnecessary line, but here for formatting.
            if(pipeline[DECODE].itype == BRANCH || pipeline[ALU].itype == BRANCH){
              if(pipeline[DECODE].itype == BRANCH){
                //Check the registers of the rtype if we have an branch in decode
                if(pipeline[MEM].stage.lw.dest_reg == pipeline[DECODE].stage.branch.reg1 ||
                   pipeline[MEM].stage.lw.dest_reg == pipeline[DECODE].stage.branch.reg2) {
                   canForward = 1;
                }
              }
              if(pipeline[ALU].itype == BRANCH){
                //Check the registers of the rtype if we have an branch in alu
                if(pipeline[MEM].stage.lw.dest_reg == pipeline[ALU].stage.branch.reg1 ||
                   pipeline[MEM].stage.lw.dest_reg == pipeline[ALU].stage.branch.reg2) {
                    canForward = 1;
                }
              }
            }
        }
        if(!data_hit && !canForward){
          pipeline_cycles += (CACHE_MISS_DELAY - 1); //If we miss the cache access, incur the penalty given.
          printf("DATA MISS:\tAddress: %X\n",pipeline[MEM].stage.lw.data_address);
          for (i = 0; i < CACHE_MISS_DELAY - 1; i++) {
              push_stages();
          }
        }
        else if (data_hit || canForward){
            printf("DATA HIT:\tAddress: %X\n",pipeline[MEM].stage.lw.data_address);
        }
    }

    /* 4. Check for SW mem acess and data miss .. add delay cycles if needed */
    if (pipeline[WRITEBACK].itype == SW) {
        cache_access++;
        data_hit = iplc_sim_trap_address(pipeline[MEM].stage.sw.data_address);
        if(!data_hit) {
            pipeline_cycles += (CACHE_MISS_DELAY - 1);
            printf("DATA MISS:\tAddress: %X\n",pipeline[MEM].stage.sw.data_address);
            for (i = 0; i < CACHE_MISS_DELAY - 1; i++) {
                push_stages();
            }
        }
        else {
            printf("DATA HIT:\tAddress: %X\n",pipeline[MEM].stage.sw.data_address);
        }
    }

    /* 5. Increment pipeline_cycles 1 cycle for normal processing */  //sam
    pipeline_cycles++;
    /* 6. push stages thru MEM->WB, ALU->MEM, DECODE->ALU, FETCH->DECODE */  //sam
    push_stages();
    // 7. This is a give'me -- Reset the FETCH stage to NOP via bezero */
    bzero(&(pipeline[FETCH]), sizeof(pipeline_t));
}
