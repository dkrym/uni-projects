%% Pr. 1
I1 = imread('xkrymd00.bmp'); %zakladni obrazek

filter1 = [-0.5 -0.5 -0.5; -0.5 5.0 -0.5; -0.5 -0.5 -0.5];
I1res = imfilter(I1,filter1);
imwrite(I1res,'step1.bmp');

%% Pr. 2
for (x = 1:512)
  for (y = 1:512)
    I2res(x,y) = I1res(x,513-y); % od konce
  end;
end;
imwrite(I2res,'step2.bmp');

%% Pr. 3
filter3 = [5 5];
I3res = medfilt2(I2res,filter3);
imwrite(I3res,'step3.bmp');

%% Pr. 4
filter4 = [1 1 1 1 1; 1 3 3 3 1; 1 3 9 3 1;1 3 3 3 1;1 1 1 1 1]/49;
I4res = imfilter(I3res,filter4);
imwrite(I4res,'step4.bmp');

%% Pr. chyba
noise = 0;
I1flip = fliplr(I1); % otoceni puvodniho obrazku
I5dbl = double(im2uint8(I4res));
I1dbl = double(im2uint8(I1flip));

for (x = 1:512)
  for (y = 1:512)
    noise = noise+double(abs(I1dbl(x,y)-I5dbl(x,y)));
  end;
end;
noise=noise/512/512

%% Pr. 5
I4min = double(min(min(I4res)));
I4max = double(max(max(I4res)));
I5res = imadjust(I4res,[I4min/255;I4max/255]);
imwrite(I5res,'step5.bmp');

%% Pr. odchylka
meannohist = mean2(double(I4res))
stdnohist = std2(double(I4res))
meanhist = mean2(double(I5res))
stdhist = std2(double(I5res))

%% Pr. 6
% kvantizace obrazku
x = size(I5res);
N = 2; % 2 bity
a = 0;
b = 255;
I5dbl = double(I5res);
IQ = zeros(x(1),x(2));
for k = 1:x(1)
  for l = 1:x(2)
    IQ(k,l) = round(((2^N)-1)*(double(I5dbl(k,l))-a)/(b-a))*(b-a)/((2^N)-1) + a;
  end
end
I6res = uint8(IQ);
imwrite(I6res,'step6.bmp');


%{
% testovaci vystup
I1xt = imread('step1x.bmp');
I1t = imread('step1.bmp');
I1 = sum(sum(abs(double(I1xt) - double(I1t)))) 
%%
I2xt = imread('step2x.bmp');
I2t = imread('step2.bmp');
I2 = sum(sum(abs(double(I2xt) - double(I2t)))) 
%%
I3xt = imread('step3x.bmp');
I3t = imread('step3.bmp');
I3 = sum(sum(abs(double(I3xt) - double(I3t)))) 
%%
I4xt = imread('step4x.bmp');
I4t = imread('step4.bmp');
I4 = sum(sum(abs(double(I4xt) - double(I4t)))) 
%%
I5xt = imread('step5x.bmp');
I5t = imread('step5.bmp');
I5 = sum(sum(abs(double(I5xt) - double(I5t)))) 
%%
I6xt = imread('step6x.bmp');
I6t = imread('step6.bmp');
I6 = sum(sum(abs(double(I6xt) - double(I6t)))) 
%%
%}

