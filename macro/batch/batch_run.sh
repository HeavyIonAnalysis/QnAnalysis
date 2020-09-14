#!/bin/bash
#SBATCH -J Flow
#SBATCH -o out/%j.out.log
#SBATCH -e error/%j.err.log
#SBATCH --time=0:20:00
#SBATCH --array=0-1

source /lustre/cbm/users/klochkov/soft/root/root6/v6-18.04_c++11/bin/thisroot.sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/lustre/cbm/users/klochkov/soft/flow/install/lib:/lustre/cbm/users/klochkov/soft/flow/install/external/lib/:/lustre/cbm/users/klochkov/soft/analysis_tree/install_c++11/lib/
EXE_DIR=/lustre/cbm/users/klochkov/soft/flow/install/bin/

INDEX=$SLURM_ARRAY_TASK_ID
INDEX=$(printf "%03d" "$INDEX")

IN_DIR=/lustre/cbm/users/klochkov/cbm/oct19_fr_18.2.1_fs_jun19p1/dcmqgsm_smm_pluto/auau/12agev/mbias/psd44_hole20_pipe0/TGeant4

FILELIST=$IN_DIR/filelists/rec/filelist_$INDEX
FILELIST_ANA=$IN_DIR/filelists/ana/filelist_$INDEX

OUT_DIR=$IN_DIR/flow/

mkdir -p $OUT_DIR
mkdir -p $OUT_DIR/logs

cd $OUT_DIR
mkdir $INDEX
cd $INDEX

ls -d $IN_DIR/filler/$INDEX.analysistree.root > filelist_temp.txt


for CORR_STEP in `seq 0 2`;
  do
    CORR_FILE=corrections_$(($CORR_STEP-1)).root    
    $EXE_DIR/correct $FILELIST $CORR_FILE filelist_temp.txt
    mv qn_vectors.root qn_vectors_$CORR_STEP.root
    mv corrections.root corrections_$CORR_STEP.root
  done

for CORR_STEP in `seq 0 2`;
  do
    ls -d $OUT_DIR/$INDEX/qn_vectors_$CORR_STEP.root > filelist_ana_$CORR_STEP.txt
    $EXE_DIR/correlate filelist_ana_$CORR_STEP.txt &> $OUT_DIR/$INDEX/log_correlate_$CORR_STEP.txt
    mv correlation.root correlation_$CORR_STEP.root
  done
