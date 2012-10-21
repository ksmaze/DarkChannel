img_name='tiananmen1_trans.bmp';
scribs_img_name='tiananmen1.bmp';

thr_alpha=[];
epsilon=[];
win_size=[];
levels_num=1;
active_levels_num=1;
sig=0.1^6;

I=double(imread(img_name))/255;
mI=double(rgb2gray(imread(scribs_img_name)))/255;
consts_map=sum(abs(I-mI),3)>0.001;
consts_vals=mI.*consts_map;

alpha=solveAlphaC2F(I,consts_map,consts_vals,levels_num, ...
                    active_levels_num,thr_alpha,epsilon,win_size);
[F,B]=solveFB(I,alpha);

imwrite(B.*repmat(1-alpha,[1,1,1]), 'tiananmen1_trans2.bmp');
